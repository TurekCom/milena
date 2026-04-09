static double my_strtod(char *str,char **ptr)
{
	int sign=1;
	double rc,rd;
	if (*str=='+') {
		str++;
	}
	else if (*str=='-') {
		sign=-1;
		str++;
	}
	rc=0.0;
	while (*str && isdigit(*str)) {
		rc=(10.0 * rc) + ((*str++) - '0');
	}
	if (*str == '.' || *str == ',') {
		str++;
		rd=1.0;
		while (*str && isdigit(*str)) {
			rd=rd * 0.1;
			rc += rd * ((*str++) - '0');
		}
	}
	if (ptr) *ptr=str;
	
	return rc * sign;
}
