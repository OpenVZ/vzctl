#!/bin/bash

if test $# -ne 1; then
	echo "Usage:" `basename $0` "manfile.ps" 1>&2
	exit 0;
fi

if ! test -f $1; then
	echo "File not found: $1" 1>&2
	exit 1;
fi

grep -A5 "^%%Page:" $1 | tr '\\' '*' | sed "s/*214/fi/" | awk '
BEGIN {
    p=0
    prevname="no";
}
/^%%Page: / {
    p=$2;
}
/\/F0 10.95\/Times-Roman\@0 SF/ {
  i=0; l=0; sn=3;
  do {
    if (l>0)
    {
        getline;
	sn=0;
    }
    for (n=sn; n <= NF; n++)
    {
	prev="";
        for(x=1; x<=length($n); x++)
        {
	    c=substr($n, x, 1);
	    if (i==0)
	    {
		if (c == "(")
		{
		    i=1;
		    continue;
		}

	    }
	    else if (i==1)
	    {
		if (c == ")")
		{
		    if (prev == "*")
		    {
			name=name ")"
			i=2;
		    }
		    else
			i=0;
		}
	    }
	    if ((i==1) && (c != "*"))
		name=name c;
	    prev=c;
	}
    }
    l++;
  } while ( substr(name, length(name), 1) != ")" );

  if (name != prevname)
  {
    print name " 	 " p+1;
    print ".br"
  }
  prevname=name;
  name="";
}'
