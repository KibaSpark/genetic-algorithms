"""reference.py — Implementación de referencia de NL y WHS en Python (lenta pero clara).

Esta es la primera pieza que implementas. Sirve para dos cosas:
  1. Entender las fórmulas antes de pelearte con C++.
  2. Verificar después que el C++ calcula lo mismo (crosscheck.py la usa).

Correr:  python python/reference.py   -> prueba con la S-box de AES (debe dar NL = 112)
"""

import numpy as np

AES_HEX = (
    "637c777bf26b6fc53001672bfed7ab76ca82c97dfa5947f0add4a2af9ca472c0"
    "b7fd9326363ff7cc34a5e5f171d8311504c723c31896059a071280e2eb27b275"
    "09832c1a1b6e5aa0523bd6b329e32f8453d100ed20fcb15b6acbbe394a4c58cf"
    "d0efaafb434d338545f9027f503c9fa851a3408f929d38f5bcb6da2110fff3d2"
    "cd0c13ec5f974417c4a77e3d645d197360814fdc222a908846eeb814de5e0bdb"
    "e0323a0a4906245cc2d3ac629195e479e7c8376d8dd54ea96c56f4ea657aae08"
    "ba78252e1ca6b4c6e8dd741f4bbd8b8a703eb5664803f60e613557b986c11d9e"
    "e1f8981169d98e949b1e87e9ce5528df8ca1890dbfe6426841992d0fb054bb16"
)
# Valor exacto de WHS(AES) con R = 12, X = 0 (suma de enteros, calculado aparte).
AES_WHS_EXACT = 2780281712723519078400


def sbox_from_hex(hex_str: str) -> np.ndarray:
    """512 caracteres hex -> arreglo de 256 enteros."""
    return np.frombuffer(bytes.fromhex(hex_str), dtype=np.uint8).astype(np.int64)


def walsh_table(s: np.ndarray) -> np.ndarray:
    """TODO(A): regresa W con forma (255, 256), donde W[b-1, a] corresponde a b = 1..255.

    W[b][a] = sum_x (-1)^( paridad(b & S[x]) XOR paridad(a & x) )

    Pistas (en NumPy, sin ciclos sobre a):
      - Construye una matriz F de (255, 256) con F[b-1, x] = (-1)^paridad(b & S[x]).
        Para la paridad: np.bitwise_count (NumPy >= 2.0) da el número de bits en 1; % 2.
        TRAMPA: np.bitwise_count regresa uint8, y en uint8 1 - 2*1 = 255, no -1.
        Conviértelo a int64 (.astype(np.int64)) antes de hacer la cuenta.
      - La matriz de Hadamard H[x, a] = (-1)^paridad(x & a) ya existe: scipy.linalg.hadamard(256)
        (con el orden de Sylvester, que es justo este).
      - Entonces W = F @ H.  Piensa por qué: es la misma suma, escrita como producto de matrices.
    """
    raise NotImplementedError("TODO(A): implementa walsh_table en python/reference.py")


def nonlinearity(W: np.ndarray) -> int:
    """TODO(B): NL = 128 - max|W| / 2   (W ya excluye b = 0)."""
    raise NotImplementedError("TODO(B): implementa nonlinearity en python/reference.py")


def whs(W: np.ndarray, R: int = 12, X: int = 0) -> float:
    """TODO(C): WHS = sum | |W| - X |^R.

    Ojo: en NumPy con int64, |W|^12 se DESBORDA sin avisar (48^12 ~ 1.5e20 > 9.2e18).
    Conviértelo a float64 antes de elevar. Si quieres el valor exacto, usa enteros de
    Python (dtype=object), que no tienen límite; es más lento pero sirve para validar.
    """
    raise NotImplementedError("TODO(C): implementa whs en python/reference.py")


def evaluate(s: np.ndarray, R: int = 12, X: int = 0) -> tuple[int, float]:
    W = walsh_table(s)
    return nonlinearity(W), whs(W, R, X)


if __name__ == "__main__":
    aes = sbox_from_hex(AES_HEX)
    nl, cost = evaluate(aes)
    print(f"NL(AES)  = {nl}   (esperado 112)")
    print(f"WHS(AES) = {cost:.6e}   (esperado {float(AES_WHS_EXACT):.6e})")
    ok = nl == 112 and abs(cost - AES_WHS_EXACT) / AES_WHS_EXACT < 1e-12
    print("OK: tu referencia está bien" if ok else "Algo no cuadra, revisa las fórmulas")
