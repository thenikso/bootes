MACRO Debug.Print.Number num
{
	local d0,d1,d2,d3
	
	d0 = '0'+ num shr 12 and 0Fh
	d1 = '0'+ num shr 8 and 0Fh
	d2 = '0'+ num shr 4 and 0Fh
	d3 = '0'+ num and 0Fh
	
	if d1>'9'
	d1 = d1 + 7
	end if
	if d2>'9'
	d2 = d2 + 7
	end if
	if d3>'9'
	d3 = d3 + 7
	end if
	
	display '0x',d0,d1,d2,d3
}

MACRO Debug.Print.String [params]
{
	common
		display " ! "
	forward
		if params eqtype ""
			display params
		else
			Debug.Print.Number params
		end if
	common
		display 13,10
}

MACRO Debug.Print.Addr info
{
	Debug.Print.String info, ": ", $
}

MACRO Debug.Print.Breakpoint
{
	Debug.Print.Addr "BREAKPOINT"
}