TESTS = test.sh

EXTRA_DIST = regexp_test.nsc io_test.nsc math_test.nsc			\
	math_test_new.dat math_test_old.dat iarray_test.nsc		\
	darray_test.nsc sarray_test.nsc string_test.nsc $(TESTS)
NGRAPH=../src/ngraph

test: math_test io_test regexp_test iarray_test darray_test sarray_test string_test

string_test: string_test.nsc
	$(NGRAPH) -i string_test.nsc

iarray_test: iarray_test.nsc
	$(NGRAPH) -i iarray_test.nsc

darray_test: darray_test.nsc
	$(NGRAPH) -i darray_test.nsc

sarray_test: sarray_test.nsc
	$(NGRAPH) -i sarray_test.nsc

io_test: io_test.nsc
	$(NGRAPH) -i io_test.nsc

regexp_test: regexp_test.nsc
	$(NGRAPH) -i regexp_test.nsc

math_test: math_test.nsc math_test_old.dat  math_test_new.dat
	$(NGRAPH) -i math_test.nsc math_test_old.dat math_test_new.dat

# original Ngraph causes following errors
#
# error E
#     expected: 2.718281828459045e+00
#       result: 2.718281828459050e+00
#   difference: 4.884981308350689e-15

# error UNDEF
#     expected: undefined
#       result: undifined
# shell: dexpr: illegal type of argument
#   difference:

# error ASIN(1/2)
#     expected: 5.235987755982989e-01
#       result: 5.235987755982988e-01
#   difference: -1.110223024625157e-16

# error EI(1/2)
#     expected: 4.542199048631735e-01
#       result: 4.542199048631737e-01
#   difference: 2.220446049250313e-16

# error EQ(0.1,BREAK)
#     expected: 0.000000000000000e+00
#       result: 1.000000000000000e-01
#   difference: 1.000000000000000e-01

# error EQ(0.1,CONT)
#     expected: 0.000000000000000e+00
#       result: 1.000000000000000e-01
#   difference: 1.000000000000000e-01

# error NEQ(0.1,BREAK)
#     expected: 0.000000000000000e+00
#       result: 1.000000000000000e-01
#   difference: 1.000000000000000e-01

# error NEQ(0.1,CONT)
#     expected: 0.000000000000000e+00
#       result: 1.000000000000000e-01
#   difference: 1.000000000000000e-01

# error OR(1,break)
#     expected: 0.000000000000000e+00
#       result: 1.000000000000000e+00
#   difference: 1.000000000000000e+00

# error OR(1,cont)
#     expected: 0.000000000000000e+00
#       result: 1.000000000000000e+00
#   difference: 1.000000000000000e+00

# error AND(undef,1)
#     expected: undefined
#       result: undifined
# shell: dexpr: illegal type of argument
#   difference:

# error AND(1,undef)
#     expected: undefined
#       result: undifined
# shell: dexpr: illegal type of argument
#   difference:

# error AND(0,undef)
#     expected: undefined
#       result: undifined
# shell: dexpr: illegal type of argument
#   difference:
# shell: dexpr: illegal type of argument

# error dif(2)
#     expected: undefined
#       result:
# shell: dexpr: illegal type of argument
#   difference:
# shell: dexpr: illegal type of argument

# error sum(2)
#     expected: 2.000000000000000e+00
#       result:
#   difference: -2.000000000000000e+00
# shell: dexpr: illegal type of argument

# error FOR(0,1,10,1,sum(1))
#     expected: 1.000000000000000e+01
#       result:
#   difference: -1.000000000000000e+01
