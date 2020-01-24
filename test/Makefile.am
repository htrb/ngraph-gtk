TESTS = math_test.sh math_test_utf8.sh string_test.sh iarray_test.sh	\
	darray_test.sh sarray_test.sh io_test.sh regexp_test.sh

EXTRA_DIST = math_test.nsc math_test_utf8.dat math_test_new.dat		\
	string_test.nsc iarray_test.nsc darray_test.nsc			\
	sarray_test.nsc io_test.nsc regexp_test.nsc math_test_old.dat	\
	test.sh $(TESTS)

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
