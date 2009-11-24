NGRAPH=../src/ngraph
#NGRAPH=mngraph

math_test: math_test_new math_test_old

math_test_new: math_test_new.nsc
	$(NGRAPH) -i math_test_new.nsc

math_test_new.nsc: math_test_new.in
	ruby math_test.rb math_test_new.in > math_test_new.nsc

math_test_old: math_test_old.nsc
	$(NGRAPH) -i math_test_old.nsc

math_test_old.nsc: math_test_old.in
	ruby math_test.rb math_test_old.in > math_test_old.nsc


# original Ngraph causes following errors
#
# error E 2.718281828459045e+00 2.718281828459050e+00.
# error UNDEF undefined undifined.
# error ASIN(1/2) 5.235987755982989e-01 5.235987755982988e-01.
# error EI(1/2) 4.542199048631735e-01 4.542199048631737e-01.
# error EQ(0.1,BREAK) 0.000000000000000e+00 1.000000000000000e-01.
# error EQ(0.1,CONT) 0.000000000000000e+00 1.000000000000000e-01.
# error NEQ(0.1,BREAK) 0.000000000000000e+00 1.000000000000000e-01.
# error NEQ(0.1,CONT) 0.000000000000000e+00 1.000000000000000e-01.
# error OR(1,break) 0.000000000000000e+00 1.000000000000000e+00.
# error OR(1,cont) 0.000000000000000e+00 1.000000000000000e+00.
# error AND(undef,1) undefined undifined.
# error AND(1,undef) undefined undifined.
# error AND(0,undef) undefined undifined.
# shell: dexpr: illegal type of argument
# error sum(2) 2.000000000000000e+00 .
# shell: dexpr: illegal type of argument
# error FOR(0,1,10,1,sum(1)) 1.000000000000000e+01 .
