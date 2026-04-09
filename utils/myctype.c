/*
 * myctype.c - Milena TTS system utilities
 * Copyright (C) Bohdan R. Rau 2008 <ethanak@polip.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write see:
 *               <http://www.gnu.org/licenses/>.
 */

static char ltrs[256]={
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,
	0,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
	12,12,12,12,12,12,12,12,12,12,12,0,0,0,0,0,
	0,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
	4,4,4,4,4,4,4,4,4,4,4,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,12,0,12,0,12,12,0,0,12,12,12,12,0,12,12,
	0,4,0,4,0,4,4,0,0,4,4,4,4,0,4,4,
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
	12,12,12,12,12,12,12,0,12,12,12,12,12,12,12,4,
	4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
	4,4,4,4,4,4,4,0,4,4,4,4,4,4,4,0};

int my_alpha(int n)
{
	return ltrs[n & 255] & 4;
}

int my_upper(int n)
{
	return ltrs[n & 255] & 8;
}

int my_alnum(int n)
{
	return ltrs[n & 255] & 6;
}

int my_space(int n)
{
	return ltrs[n & 255] & 1;

}

int my_digit(int n)
{
	return ltrs[n & 255] & 2;

}

int readable(char *s)
{
	for (;*s;s++) if (my_alnum(*s)) return 1;
	return 0;
}
