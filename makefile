version=0.9.2

opt:
	make all CPPFLAGS="-DNDEBUG -std=c++11 -msse4.2 -funroll-loops -O3"

assert:
	make all CPPFLAGS="-std=c++11 -O3"
	
debug:
	make all CPPFLAGS="-g -std=c++11 -DDEBUG"

all:
	g++ $(CPPFLAGS) testcount64.cpp -o testcount64
	g++ $(CPPFLAGS) testselect64.cpp -o testselect64
	g++ $(CPPFLAGS) -DCLASS=jacobson -DNOSELECTTEST jacobson.cpp testranksel.cpp -o testjacobson
	g++ $(CPPFLAGS) -DCLASS=rank9b -DNOSELECTTEST rank9b.cpp testranksel.cpp -o testrank9b
	g++ $(CPPFLAGS) -DCLASS=simple_select -DNORANKTEST -DMAX_LOG2_LONGWORDS_PER_SUBINVENTORY=0 rank9.cpp simple_select.cpp testranksel.cpp -o testsimplesel0
	g++ $(CPPFLAGS) -DCLASS=simple_select -DNORANKTEST -DMAX_LOG2_LONGWORDS_PER_SUBINVENTORY=1 rank9.cpp simple_select.cpp testranksel.cpp -o testsimplesel1
	g++ $(CPPFLAGS) -DCLASS=simple_select -DNORANKTEST -DMAX_LOG2_LONGWORDS_PER_SUBINVENTORY=2 rank9.cpp simple_select.cpp testranksel.cpp -o testsimplesel2
	g++ $(CPPFLAGS) -DCLASS=simple_select -DNORANKTEST -DMAX_LOG2_LONGWORDS_PER_SUBINVENTORY=3 rank9.cpp simple_select.cpp testranksel.cpp -o testsimplesel3
	g++ $(CPPFLAGS) -DCLASS=simple_rank -DNOSELECTTEST simple_rank.cpp testranksel.cpp -o testsimplerank
	g++ $(CPPFLAGS) -DCLASS=simple_select_half -DNORANKTEST rank9.cpp simple_select_half.cpp testranksel.cpp -o testsimplehalf
	g++ $(CPPFLAGS) -DCLASS=elias_fano rank9.cpp simple_select_half.cpp simple_select_zero_half.cpp elias_fano.cpp testranksel.cpp -o testeliasfano
	g++ $(CPPFLAGS) -DCLASS=rank9sel rank9sel.cpp testranksel.cpp -o testrank9sel
	g++ $(CPPFLAGS) -DCLASS=rank9sel -DNORANKTEST -DSELPOPCOUNT rank9sel.cpp testranksel.cpp -o testrank9selpc
	g++ $(CPPFLAGS) -DCLASS=rank9sel -DSELPOPCOUNT rank9sel.cpp testranksel.cpp -o testrank9selpcu
	g++ $(CPPFLAGS) -DPOSITIONS=10000000 -DREPEATS=10 rank9.cpp bal_paren.cpp testbalparen.cpp -o testbalparen
	g++ $(CPPFLAGS) -DPOSITIONS=10000000 -DREPEATS=10 -DSLOW_NO_TABS rank9.cpp bal_paren.cpp testbalparen.cpp -o testbalparenfl

ext:
	cd bitarray; g++ -m32 -O3 bitselect.cpp bitarray.cpp testbitarray.cpp -o testbitarray; mv testbitarray ..; cd ..
	cd kim; g++ -m32 -O3 sucBV_unitTest.cpp -o testkim; mv testkim ..; cd ..
	cd sadakane; gcc -m32 -O3 -DNDEBUG select2.c -o select2; gcc -m32 -O3 -DNDEBUG selectd.c -o selectd; mv select2 selectd ..; cd ..

source: 
	-rm -fr sux-$(version)
	ln -s . sux-$(version)
	./genz.sh
	tar zcvf sux-$(version).tar.gz --owner=0 --group=0 \
		sux-$(version)/makefile \
		sux-$(version)/README \
		sux-$(version)/CHANGES \
		sux-$(version)/COPYING \
		sux-$(version)/COPYING.LESSER \
		sux-$(version)/testbalparen.cpp \
		sux-$(version)/testranksel.cpp \
		sux-$(version)/test*64.cpp \
		sux-$(version)/posrep.h \
		sux-$(version)/select.h \
		sux-$(version)/rank9*.cpp \
		sux-$(version)/rank9*.h \
		sux-$(version)/simple_*.cpp \
		sux-$(version)/simple_*.h \
		sux-$(version)/elias_fano.cpp \
		sux-$(version)/elias_fano.h \
		sux-$(version)/jacobson.cpp \
		sux-$(version)/jacobson.h \
		sux-$(version)/popcount.h \
		sux-$(version)/bal_paren.h \
		sux-$(version)/bal_paren.cpp \
		sux-$(version)/posrep.h \
		sux-$(version)/macros.h \
		sux-$(version)/tables.h
	rm sux-$(version)
