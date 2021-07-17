#ifndef MAIN_H
#define MAIN_H

#pragma once

#define CLOSED 0
#define OPENED 1
#define MAGIC_MARK(X, Y) ((X << 1) | (Y & 1))
#define GET_REAL(X) X>>1
#define IS_TO_RESET(X) X % 2 == 1

#endif