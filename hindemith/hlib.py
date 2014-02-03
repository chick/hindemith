from numpy import *
import scipy.sparse
import scipy.sparse.linalg


def red(v):
    o = 0.0 * v
    o[::2, ::2] = v[::2, ::2]
    o[1::2, 1::2] = v[1::2, 1::2]
    return o


def black(v):
    o = 1.0 * v
    o[::2, ::2] = 0.0
    o[1::2, 1::2] = 0.0
    return o


def applyHSPreconditioner(D, diag0, diag1, offdiag, x0, x1):
    height, width = x0.shape
    dtype = x0.dtype
    a = zeros((height, width), dtype=dtype)
    b = zeros((height, width), dtype=dtype)
    c = zeros((height, width), dtype=dtype)
    d = zeros((height, width), dtype=dtype)
    a += diag0
    d += diag1
    b += offdiag
    c += offdiag
    a[1:, :] += D.data[2]
    d[1:, :] += D.data[2]
    a[:-1, :] += D.data[2]
    d[:-1, :] += D.data[2]
    a[:, 1:] += D.data[2]
    d[:, 1:] += D.data[2]
    a[:, :-1] += D.data[2]
    d[:, :-1] += D.data[2]

    det = (a * d) - (c * b)
    mask = abs(det) > 1e-4
    g = (x0 * d - b * x1) / det
    h = (-c * x0 + a * x1) / det
    y0 = 1.0 * x0
    y1 = 1.0 * x1
    y0[mask] = g[mask]
    y1[mask] = h[mask]

    return y0, y1


def applyBroxPreconditioner(D, diag0, diag1, offdiag, x0, x1):
    height, width = x0.shape
    dtype = x0.dtype
    a = zeros((height, width), dtype=dtype)
    b = zeros((height, width), dtype=dtype)
    c = zeros((height, width), dtype=dtype)
    d = zeros((height, width), dtype=dtype)
    a += diag0 + D.data[2, :, :]
    b += offdiag
    c += offdiag
    d += diag1 + D.data[2, :, :]
    det = (a * d) - (c * b)
    mask = abs(det) > 1e-4
    g = (x0 * d - b * x1) / det
    h = (-c * x0 + a * x1) / det
    y0 = 1.0 * x0
    y1 = 1.0 * x1
    y0[mask] = g[mask]
    y1[mask] = h[mask]

    return y0, y1


def set_brox_matrix(A, PsiSmooth, brox_alpha):
    (ndiags, height, width) = A.data.shape
    A.data[:, :, :] = 0.0
    A.data[0, 1:, :] += -brox_alpha * PsiSmooth[:height - 1, :]
    A.data[2, 1:, :] += brox_alpha * PsiSmooth[:height - 1, :]
    A.data[1, :, 1:] += -brox_alpha * PsiSmooth[:, :width - 1]
    A.data[2, :, 1:] += brox_alpha * PsiSmooth[:, :width - 1]
    A.data[3, :, :width - 1] += -brox_alpha * PsiSmooth[:, :width - 1]
    A.data[2, :, :width - 1] += brox_alpha * PsiSmooth[:, :width - 1]
    A.data[4, :height - 1, :] += -brox_alpha * PsiSmooth[:height - 1, :]
    A.data[2, :height - 1, :] += brox_alpha * PsiSmooth[:height - 1, :]


def sum2d(x):
    return sum(x.flatten())


def lk_least_squares(ErrorImg, WarpedIx, WarpedIy):
    du = zeros(ErrorImg.shape)
    dv = zeros(ErrorImg.shape)

    # Compute matrix and rhs
    r = 5
    for i in range(r, ErrorImg.shape[0] - r - 1):
        for j in range(r, ErrorImg.shape[1] - r - 1):
            IxPatch = WarpedIx[i - r: i + r + 1, j - r: j + r + 1]
            IyPatch = WarpedIy[i - r: i + r + 1, j - r: j + r + 1]
            ErrPatch = ErrorImg[i - r: i + r + 1, j - r: j + r + 1]

            a = sum(IxPatch * IxPatch)
            b = sum(IxPatch * IyPatch)
            c = b
            d = sum(IyPatch * IyPatch)
            e = sum(IxPatch * ErrPatch)
            f = sum(IyPatch * ErrPatch)

            # Solve using Cramer's rule
            mdet = (a * d) - (b * c)
            if (mdet != 0):
                g = (e * d - b * f) / mdet
                h = (-c * e + a * f) / mdet
            else:
                g = 0
                h = 0
                #if(fabs(g) > 50 or fabs(h) > 50):
                #import pdb; pdb.set_trace()
            du[i, j] = g
            dv[i, j] = h
    return du, dv


def warp_img2d(src, u, v):
    dst = zeros(src.shape)
    for i in range(src.shape[0]):
        for j in range(src.shape[1]):
            u_int = int(u[i, j])
            v_int = int(v[i, j])
            us = sign(u[i, j])
            vs = sign(v[i, j])
            if ((v_int + i + vs >= 0) and (v_int + i + vs < src.shape[0]) and (u_int + j + us >= 0) and (
                        u_int + j + us < src.shape[1])):
                xfrac = (u[i, j] - u_int) * (1.0 - us)
                yfrac = (v[i, j] - v_int) * (1.0 - vs)
                dst[i, j] = 0.0
                dst[i, j] += (1.0 - xfrac) * (1.0 - yfrac) * src[v_int + i, u_int + j]
                dst[i, j] += (xfrac) * (1.0 - yfrac) * src[v_int + i + vs, u_int + j]
                dst[i, j] += (1.0 - xfrac) * (yfrac) * src[v_int + i, u_int + j + us]
                dst[i, j] += (xfrac) * (yfrac) * src[v_int + i + vs, u_int + j + us]
            else:
                dst[i, j] = src[i, j]
    return dst


class SpMat2D:
    def __init__(self, data, offx, offy):
        self.ndiags, self.dim0, self.dim1 = data.shape
        self.data = data # 3D ndarray
        self.dtype = data.dtype
        self.offx = offx
        self.offy = offy

    def __mul__(a, b):
        if a.__class__.__name__ != 'SpMat2D':
            import pdb;

            pdb.set_trace()
        if b.__class__.__name__ != 'ndarray':
            import pdb;

            pdb.set_trace()
        (height, width) = b.shape
        ovec = zeros((height, width), dtype=b.dtype)
        for diag in range(a.ndiags):
            xo = a.offx[diag]
            yo = a.offy[diag]
            d = a.data[diag]
            top = -min(yo, 0)
            bottom = max(yo, 0)
            left = -min(xo, 0)
            right = max(xo, 0)
            ovec[top:height - bottom, left:width - right] += d[top:height - bottom, left:width - right] * b[
                                                                                                          bottom:height - top,
                                                                                                          right:width - left]
        return ovec


class Stencil:
    def __init__(self, data, offx, offy):
        self.data = data
        self.offx = offx
        self.offy = offy
        self.dtype = data.dtype

    def __mul__(a, b):
        if a.__class__.__name__ != 'Stencil':
            import pdb;

            pdb.set_trace()
        if b.__class__.__name__ != 'ndarray':
            import pdb;

            pdb.set_trace()
        x_lb = -min(0, min(a.offx))
        x_ub = max(0, max(a.offx))
        y_lb = -min(0, min(a.offy))
        y_ub = max(0, max(a.offy))
        (height, width) = b.shape
        ovec = zeros((height, width), dtype=b.dtype)
        ivec = pad(b, ((y_lb, y_ub), (x_lb, x_ub)), 'edge')
        ndiags = len(a.data)
        for diag in range(ndiags):
            xo = a.offx[diag]
            yo = a.offy[diag]
            d = a.data[diag]
            ovec += d * ivec[y_lb + yo:height + y_ub + yo, x_lb + xo:width + x_ub + xo]
        return ovec


N_CHAN = 6
N_RANGE = 1024
TRAINING_BLOCK_SIZE = 64
N_BLOCKS = N_RANGE / TRAINING_BLOCK_SIZE
N_PULSES = 128
N_DOP = 256
TDOF = 3
N_STEERING = 16

# Array(N_CHAN*N_DOP*N_RANGE) -> Array2D(N_BLOCKS*N_DOP*N_CHAN*TDOF, TRAINING_BLOCK_SIZE)
def extract_snapshots(datacube_r, datacube_i):
    global N_BLOCKS
    global N_DOP
    global TRAINING_BLOCK_SIZE
    datacube = datacube_r + 1j * datacube_i
    snapshots = zeros((N_BLOCKS * N_DOP * TRAINING_BLOCK_SIZE, N_CHAN * TDOF), dtype=complex64)
    for block in range(N_BLOCKS):
        for dop_index in range(N_DOP):
            first_cell = block * TRAINING_BLOCK_SIZE
            last_cell = (block + 1) * TRAINING_BLOCK_SIZE
            for cell in range(first_cell, last_cell):
                for chan in range(N_CHAN):
                    for dof in range(TDOF):
                        dop = dop_index - (TDOF - 1) / 2 + dof
                        if dop < 0:
                            dop += N_DOP
                        if dop >= N_DOP:
                            dop -= N_DOP
                        snapshots[block * N_DOP * TRAINING_BLOCK_SIZE + dop_index * TRAINING_BLOCK_SIZE + (
                        cell - first_cell), chan * TDOF + dof] = datacube[chan * N_DOP * N_RANGE + dop * N_RANGE + cell]
    return real(snapshots), imag(snapshots)

# Array2D -> Array2D
def cgemm_batch(snapshots_r, snapshots_i):
    global N_BLOCKS
    global N_DOP
    global N_CHAN
    global TDOF
    snapshots = snapshots_r + 1j * snapshots_i
    covariance = zeros((N_BLOCKS * N_DOP * N_CHAN * TDOF, N_CHAN * TDOF), dtype=complex64)
    for block in range(N_BLOCKS):
        for dop in range(N_DOP):
            problem_id = block * N_DOP + dop
            mat = snapshots[TRAINING_BLOCK_SIZE * problem_id:TRAINING_BLOCK_SIZE * (problem_id + 1), :]
            cov = dot(conj(mat.T), mat).T
            covariance[(N_CHAN * TDOF) * problem_id:(N_CHAN * TDOF) * (problem_id + 1), :] = cov
    return real(covariance), imag(covariance)


def solve_batch(covariance_r, covariance_i, steering_r, steering_i):
    covariance = covariance_r + 1j * covariance_i
    steering = steering_r + 1j * steering_i
    global N_BLOCKS
    global N_DOP
    global N_CHAN
    global TDOF
    adaptive_weights = zeros((N_BLOCKS * N_DOP * N_STEERING, N_CHAN * TDOF), dtype=complex64)
    for block in range(N_BLOCKS):
        for dop in range(N_DOP):
            problem_id = dop + block * N_DOP
            mat = covariance[(N_CHAN * TDOF) * problem_id:(N_CHAN * TDOF) * (problem_id + 1), :]
            aw = linalg.solve(mat, steering.T)
            adaptive_weights[(N_STEERING) * problem_id:(N_STEERING) * (problem_id + 1), :] = aw.T
    return real(adaptive_weights), imag(adaptive_weights)

# Array2D -> Array2D
def gamma_weights(adaptive_weights_r, adaptive_weights_i, steering_vectors_r, steering_vectors_i):
    global N_BLOCKS
    global N_DOP
    global N_STEERING
    steering_vectors = steering_vectors_r + 1j * steering_vectors_i
    adaptive_weights = adaptive_weights_r + 1j * adaptive_weights_i
    gamma_weights = zeros((N_BLOCKS * N_DOP, N_STEERING), dtype=complex64)
    for dop in range(N_DOP):
        for block in range(N_BLOCKS):
            problem_id = block * N_DOP + dop
            aw = adaptive_weights[N_STEERING * (problem_id):N_STEERING * (problem_id + 1), :]
            accum = sum(conj(aw) * steering_vectors, 1)
            gamma = abs(accum)
            gamma[gamma <= 0.0] = 1.0
            gamma_weights[problem_id, :] = 1.0 / (gamma)
    return real(gamma_weights), imag(gamma_weights)

# Array2D -> Array
def inner_products(snapshots_r, snapshots_i, adaptive_weights_r, adaptive_weights_i, gamma_r, gamma_i):
    global N_BLOCKS
    global N_DOP
    global N_STEERING
    global N_CHAN
    global TDOF
    global TRAINING_BLOCK_sIZE
    adaptive_weights = adaptive_weights_r + 1j * adaptive_weights_i
    snapshots = snapshots_r + 1j * snapshots_i
    gamma = gamma_r + 1j * gamma_i
    output = zeros((N_DOP * N_RANGE, N_STEERING), dtype=complex64)
    for dop in range(N_DOP):
        for block in range(N_BLOCKS):
            first_cell = block * TRAINING_BLOCK_SIZE
            last_cell = (block + 1) * TRAINING_BLOCK_SIZE
            problem_id = dop + block * N_DOP
            ss = snapshots[TRAINING_BLOCK_SIZE * problem_id:TRAINING_BLOCK_SIZE * (problem_id + 1), :]
            aw = adaptive_weights[N_STEERING * problem_id:N_STEERING * (problem_id + 1), :]
            output[TRAINING_BLOCK_SIZE * problem_id:TRAINING_BLOCK_SIZE * (problem_id + 1), :] = dot(conj(ss), aw.T)
            for sv in range(N_STEERING):
                output[TRAINING_BLOCK_SIZE * problem_id:TRAINING_BLOCK_SIZE * (problem_id + 1), sv] *= gamma[
                    problem_id, sv]
    return real(output), imag(output)



