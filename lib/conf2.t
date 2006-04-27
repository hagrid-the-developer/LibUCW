# test config file
#include lib/conf2.t ; top.xa=1
#include 'non-existent file'; #top.xa=1
Top { \

  nr1=16	#!!!
  nrs1		2 3 5 \
	    7 11 13 \
	    \
	    17M
  nrs2	3 3k 3 3 3 ; \
  str1	"hello,\t\x2bworld%%\n"
  str2	'Hagenuk,
      the best' "\
      " qu'est-ce que c'est?
  u1	0xbadcafebadbeefc0
  str2:prepend prepended
  str2:append appended
  d1 7%
  d1	-1.14e-25
  firsttime ; secondtime 56
  ^top.master:set	alice HB8+
  slaves:clear
  ip 0xa
  ip 195.113.31.123
  look Alpha
  look:prepend Beta GAMMA
  numbers 11000 65535
};;;;;;

unknown.ignored :-)

top.slaves	cairns gpua 7 7 -10% +10%
top.slaves	daintree rafc 4 5 -171%
top.slaves	coogee pum 9 8
top.slaves:prepend	{name=bondi; level=\
  "PUG"; confidence	10 10}
top.slaves:remove {name daintree}
top.slaveS:edit {level PUG} Bondi PUG!
top.slaveS:before {level pum}{
  confidence 2
  list 123 456 789
}
top.slaves:duplicate {name coogee} Coogee2 PUM

topp.a=15
top.nr1=   ' 15'
a { ;-D }
