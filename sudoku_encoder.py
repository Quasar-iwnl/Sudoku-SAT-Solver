import sys
import itertools
from itertools import product as iprod

import numpy as np


class Literal:
    __slots__ = ('x', 'y', 'v')

    def __init__(self, x, y, v):
        assert v != 0
        self.x = x
        self.y = y
        self.v = v

    def __eq__(self, other):
        if not isinstance(other, Literal):
            return NotImplemented
        return self.x == other.x and self.y == other.y and self.v == other.v

    def __hash__(self):
        return hash((self.x, self.y, self.v))

    def __bool__(self):
        return bool(self.v > 0)

    def __invert__(self):
        return type(self)(self.x, self.y, -self.v)

    def __abs__(self):
        return self if self else ~self

    def __repr__(self):
        return '({}, {}, {})'.format(self.x, self.y, self.v)


def test_basic_encoder():
    def check001(clauses):
        for clause in clauses:
            for lit in clause:
                assert lit.x > 0 and lit.y > 0, clause
                assert lit.x <= 9 and lit.y <= 9, clause

    board = np.loadtxt('example-boards/board.txt', dtype=int, delimiter=',')
    e = BasicEncoder(board)
    check001(e.cell_def())
    check001(e.cell_uni())
    check001(e.row_def())
    check001(e.row_uni())
    check001(e.col_def())
    check001(e.col_uni())
    check001(e.block_def())
    check001(e.block_uni())


class BasicEncoder:
    """
    References::

        - https://app.cs.amherst.edu/~ccmcgeoch/cs34/papers/sudokusat.pdf
    """
    def __init__(self, board):
        assert board.shape[0] == board.shape[1]
        self.board = board
        self.n = board.shape[0]
        self.n_sqrt = int(np.sqrt(self.n))
        assert np.isclose(self.n_sqrt, np.sqrt(self.n))

    def cell_def(self):
        for x, y in iprod(range(1, self.n + 1), range(1, self.n + 1)):
            yield [Literal(x, y, v) for v in range(1, self.n + 1)]

    def cell_uni(self):
        for x, y in iprod(range(1, self.n + 1), range(1, self.n + 1)):
            for vi in range(1, self.n):
                for vj in range(vi + 1, self.n + 1):
                    yield [Literal(x, y, -vi), Literal(x, y, -vj)]

    def row_def(self):
        for x, v in iprod(range(1, self.n + 1), range(1, self.n + 1)):
            yield [Literal(x, y, v) for y in range(1, self.n + 1)]

    def row_uni(self):
        for x, v in iprod(range(1, self.n + 1), range(1, self.n + 1)):
            for yi in range(1, self.n):
                for yj in range(yi + 1, self.n + 1):
                    yield [Literal(x, yi, -v), Literal(x, yj, -v)]

    def col_def(self):
        for y, v in iprod(range(1, self.n + 1), range(1, self.n + 1)):
            yield [Literal(x, y, v) for x in range(1, self.n + 1)]

    def col_uni(self):
        for y, v in iprod(range(1, self.n + 1), range(1, self.n + 1)):
            for xi in range(1, self.n):
                for xj in range(xi + 1, self.n + 1):
                    yield [Literal(xi, y, -v), Literal(xj, y, -v)]

    def block_def(self):
        nsq = self.n_sqrt
        for xo, yo, v in iprod(range(nsq), range(nsq), range(1, self.n + 1)):
            yield [Literal(xo * nsq + x, yo * nsq + y, v)
                   for x in range(1, nsq + 1)
                   for y in range(1, nsq + 1)]

    def block_uni(self):
        nsq = self.n_sqrt
        for v, i, j, x, y in iprod(range(1, self.n + 1), range(nsq),
                                   range(nsq), range(1, nsq + 1),
                                   range(1, nsq + 1)):
            for k in range(y + 1, nsq + 1):
                yield [
                    Literal(i * nsq + x, j * nsq + y, -v),
                    Literal(i * nsq + x, j * nsq + k, -v),
                ]
            for k in range(x + 1, nsq + 1):
                for l in range(1, nsq + 1):
                    yield [
                        Literal(i * nsq + x, j * nsq + y, -v),
                        Literal(i * nsq + k, j * nsq + l, -v),
                    ]

    def assigned(self):
        return [[Literal(x, y, self.board[x - 1, y - 1])]
                for x in range(1, self.n + 1)
                for y in range(1, self.n + 1)
                if self.board[x - 1, y - 1] != 0]


class ExtendedEncoder(BasicEncoder):
    def encode(self):
        return itertools.chain(self.cell_def(), self.cell_uni(),
                               self.row_def(), self.row_uni(), self.col_def(),
                               self.col_uni(), self.block_def(),
                               self.block_uni(), self.assigned())


class MinimalEncoder(BasicEncoder):
    def encode(self):
        return itertools.chain(self.cell_def(), self.row_uni(), self.col_uni(),
                               self.block_uni(), self.assigned())


# Too slow; not used
def basic_filtering(assigned_encoding, clauses):
    """
    Reference: A Sudoku-Solver for Large Puzzles using SAT
    """
    asslit = list(itertools.chain.from_iterable(assigned_encoding))
    revlit = set(~lit for lit in asslit)
    for clause in clauses:
        clause_cache = set(clause)
        for lit in asslit:
            if lit in clause_cache:
                break
        else:
            yield [lit for lit in clause if lit not in revlit]


class VariableManager:
    def __init__(self):
        self.index = None
        self.pool = None

    def convert(self, clauses):
        self.index = [None]
        self.pool = {}
        for clause in clauses:
            cvt_clause = []
            for lit in clause:
                abs_lit = abs(lit)
                try:
                    vid = self.pool[abs_lit]
                except KeyError:
                    vid = len(self.index)
                    self.pool[abs_lit] = vid
                    self.index.append(abs_lit)
                cvt_clause.append(vid if lit else -vid)
            yield cvt_clause

    def back(self, solution):
        for signed_vid in solution:
            if signed_vid > 0:
                yield self.index[signed_vid]