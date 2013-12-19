//
// This file is automatically generated by createcc (O.Grau). Do not edit!
//


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "puttCalc.h"
#include "ErrorMsg.h"


using namespace std;

static void    MyStrDup(char * &target,const char *s)
{
	if (target!=NULL) {
		delete[] target;
		target = NULL;
	}
	const size_t l = strlen(s)+1;
	target = new char[l];
	strcpy(target,s);
}




_puttCalc_parameter :: _puttCalc_parameter() {
		is_strict=true;
		inc = 1;
		inc_flag = 0;
		campatt = 0;
		MyStrDup( campatt, "." );
		campatt_flag = 0;
		fn2 = 0;
		MyStrDup( fn2, "." );
		fn2_flag = 0;
		deint = 0;
		deint_flag = 0;
		dx = 704;
		dx_flag = 0;
		cfn = 0;
		MyStrDup( cfn, "" );
		cfn_flag = 0;
		dy = 576;
		dy_flag = 0;
		filt = 0;
		filt_flag = 0;
		fmt = 2;
		fmt_flag = 0;
		fn = 0;
		MyStrDup( fn, "." );
		fn_flag = 0;
		cam = 0;
		MyStrDup( cam, "" );
		cam_flag = 0;
		patt2 = 0;
		MyStrDup( patt2, "." );
		patt2_flag = 0;
		end = 0;
		end_flag = 0;
		outpatt = 0;
		MyStrDup( outpatt, "." );
		outpatt_flag = 0;
		start = 0;
		start_flag = 0;
		patt = 0;
		MyStrDup( patt, "." );
		patt_flag = 0;
		help_flg=0;
};

_puttCalc_parameter :: ~ _puttCalc_parameter() {
		if(campatt) delete [] campatt;
		if(fn2) delete [] fn2;
		if(cfn) delete [] cfn;
		if(fn) delete [] fn;
		if(cam) delete [] cam;
		if(patt2) delete [] patt2;
		if(outpatt) delete [] outpatt;
		if(patt) delete [] patt;
};

int
_puttCalc_parameter :: ReadInt(int &i, char *s, int min, int max)
{
	int iin;
	int rc=sscanf(s,"%d",&iin);
	if(rc!=1) {
		cerr<<"ERROR: ParseArg ReadInt failed: "<<s<<endl;
		throw bbcvp::Error("ParseArg ReadInt failed ");
	}
	if(iin<min || iin>max) {
		cerr<<"ERROR: ReadInt parameter out of range: "<<i<<" not in ["<<min<<";"<<max<<"]"<<endl;
		throw bbcvp::Error("ReadInt parameter out of range ");
	}
	i=iin;
	return rc;
}

int
_puttCalc_parameter :: ReadUShort(unsigned short &i, char *s)
{
	unsigned short iin;
	int rc=sscanf(s,"%hu",&iin);
	if(rc!=1) {
		cerr<<"ERROR: ParseArg ReadUShort failed: "<<s<<endl;
		throw bbcvp::Error("ParseArg ReadUShort failed ");
	}
	i=iin;
	return rc;
}

int
_puttCalc_parameter :: ReadShort(short &i, char *s, short min, short max)
{
	short iin;
	int rc=sscanf(s,"%hd",&iin);
	if(rc!=1) {
		cerr<<"ERROR: ParseArg ReadShort failed: "<<s<<endl;
		throw bbcvp::Error("ParseArg ReadShort failed ");
	}
	if(iin<min || iin>max) {
		cerr<<"ERROR: ReadShort parameter out of range: "<<i<<" not in ["<<min<<";"<<max<<"]"<<endl;
		throw bbcvp::Error("ReadShort parameter out of range ");
	}
	i=iin;
	return rc;
}

int
_puttCalc_parameter :: ReadShort(short &i, char *s)
{
	short iin;
	int rc=sscanf(s,"%hd",&iin);
	if(rc!=1) {
		cerr<<"ERROR: ParseArg ReadShort failed: "<<s<<endl;
		throw bbcvp::Error("ParseArg ReadShort failed ");
	}
	i=iin;
	return rc;
}

int
_puttCalc_parameter :: ReadDouble(double &i, char *s, double min, double max)
{
	double iin;
	int rc=sscanf(s,"%lf",&iin);
	if(rc!=1) {
		cerr<<"ERROR: ParseArg ReadDouble failed: "<<s<<endl;
		throw bbcvp::Error("ParseArg ReadDouble failed ");
	}
	if(iin<min || iin>max) {
		cerr<<"ERROR: ReadDouble parameter out of range: "<<i<<" not in ["<<min<<";"<<max<<"]"<<endl;
		throw bbcvp::Error("ReadDouble parameter out of range ");
	}
	i=iin;
	return rc;
}

int
_puttCalc_parameter :: ReadDouble(double &i, char *s)
{
	double iin;
	int rc=sscanf(s,"%lf",&iin);
	if(rc!=1) {
		cerr<<"ERROR: ParseArg ReadDouble failed: "<<s<<endl;
		throw bbcvp::Error("ParseArg ReadDouble failed ");
	}
	i=iin;
	return rc;
}

int
_puttCalc_parameter :: ParseArg(  int   argc, char **argv )
{
	int     args = 0;
	if(!strcmp(*argv,"-h")) {
		help_flg = 1;
		PrintHelp(stdout);
		return 1;
	}  else
	if(!strcmp(*argv,"-inc")) {
		if(argc>1) {
			args += ReadInt(inc,argv[1], 1, 1000000 ); 
			inc_flag = 1;
			return 2;
		}
		return 0;
	}  else
	if(!strcmp(*argv,"-campatt")) {
		if(argc>1) { MyStrDup(campatt,argv[1]);
			campatt_flag = 1;
			return 2; }
		return 0;
	}  else
	if(!strcmp(*argv,"-fn2")) {
		if(argc>1) { MyStrDup(fn2,argv[1]);
			fn2_flag = 1;
			return 2; }
		return 0;
	}  else
	if(!strcmp(*argv,"-deint")) {
		if(argc>1) {
			args += ReadInt(deint,argv[1], 0, 3 ); 
			deint_flag = 1;
			return 2;
		}
		return 0;
	}  else
	if(!strcmp(*argv,"-dx")) {
		if(argc>1) {
			args += ReadInt(dx,argv[1], 0, 1000000 ); 
			dx_flag = 1;
			return 2;
		}
		return 0;
	}  else
	if(!strcmp(*argv,"-cfn")) {
		if(argc>1) { MyStrDup(cfn,argv[1]);
			cfn_flag = 1;
			return 2; }
		return 0;
	}  else
	if(!strcmp(*argv,"-dy")) {
		if(argc>1) {
			args += ReadInt(dy,argv[1], 0, 1000000 ); 
			dy_flag = 1;
			return 2;
		}
		return 0;
	}  else
	if(!strcmp(*argv,"-filt")) {
		if(argc>1) {
			args += ReadInt(filt,argv[1], 0, 3 ); 
			filt_flag = 1;
			return 2;
		}
		return 0;
	}  else
	if(!strcmp(*argv,"-fmt")) {
		if(argc>1) {
			args += ReadInt(fmt,argv[1], 0, 3 ); 
			fmt_flag = 1;
			return 2;
		}
		return 0;
	}  else
	if(!strcmp(*argv,"-fn")) {
		if(argc>1) { MyStrDup(fn,argv[1]);
			fn_flag = 1;
			return 2; }
		return 0;
	}  else
	if(!strcmp(*argv,"-cam")) {
		if(argc>1) { MyStrDup(cam,argv[1]);
			cam_flag = 1;
			return 2; }
		return 0;
	}  else
	if(!strcmp(*argv,"-patt2")) {
		if(argc>1) { MyStrDup(patt2,argv[1]);
			patt2_flag = 1;
			return 2; }
		return 0;
	}  else
	if(!strcmp(*argv,"-end")) {
		if(argc>1) {
			args += ReadInt(end,argv[1], 0, 1000000 ); 
			end_flag = 1;
			return 2;
		}
		return 0;
	}  else
	if(!strcmp(*argv,"-outpatt")) {
		if(argc>1) { MyStrDup(outpatt,argv[1]);
			outpatt_flag = 1;
			return 2; }
		return 0;
	}  else
	if(!strcmp(*argv,"-start")) {
		if(argc>1) {
			args += ReadInt(start,argv[1], 0, 1000000 ); 
			start_flag = 1;
			return 2;
		}
		return 0;
	}  else
	if(!strcmp(*argv,"-patt")) {
		if(argc>1) { MyStrDup(patt,argv[1]);
			patt_flag = 1;
			return 2; }
		return 0;
	}  else {
		cerr<<"ERROR: unrecognised argument: "<<*argv<<endl;
		throw bbcvp::Error("unrecognised argument: ", *argv);
	}
}

int _puttCalc_parameter :: ParseStr(const char *s)
{
#define MAXARG  200
	char    *buf = new char[strlen(s)+1];
	char    *wp,*sarray[MAXARG];
	int     n;
	int     w_flag=0,s_flag=0;
	int     rc;

	strcpy(buf,s);
	wp=buf;
	for(n=0; *wp!='\0' && n<MAXARG; ++wp) {
		if( *wp=='\"' ) {
			if(s_flag) s_flag = 0;
			else { s_flag = 1; w_flag = 0;}
			*wp = '\0';
		} else
		if( !s_flag && (*wp=='\t' || *wp==' ' || *wp=='\n') ) {
			*wp = '\0';
			w_flag = 0;
		} else if(!w_flag) {
			sarray[n++] = wp;
			++w_flag;
		}
	}
	rc = 0;
	for(int i=0; i<n; ) {
		rc = ParseArg(  n-i, sarray+i );
		if(rc<=0) break;
		i += rc;
	}
	delete [] buf;
	return rc;
}

int _puttCalc_parameter :: ReadConfig( FILE *fp )
{
	char    buf[2000];

	buf[0]='\0';
	for(; fgets(buf,2000,fp)!=0; ) {
		if(buf[0]=='#') continue;
		ParseStr(buf);
	}
	 return 1;
}

void
_puttCalc_parameter :: Print(  FILE *fp )
{
	if(cam && cam_flag) fprintf(fp,"-cam\t\"%s\"\n",cam);
	if(campatt && campatt_flag) fprintf(fp,"-campatt\t\"%s\"\n",campatt);
	if(cfn && cfn_flag) fprintf(fp,"-cfn\t%s\n",cfn);
	if(deint_flag) fprintf(fp,"-deint\t%d\n",deint);
	if(dx_flag) fprintf(fp,"-dx\t%d\n",dx);
	if(dy_flag) fprintf(fp,"-dy\t%d\n",dy);
	if(end_flag) fprintf(fp,"-end\t%d\n",end);
	if(filt_flag) fprintf(fp,"-filt\t%d\n",filt);
	if(fmt_flag) fprintf(fp,"-fmt\t%d\n",fmt);
	if(fn && fn_flag) fprintf(fp,"-fn\t\"%s\"\n",fn);
	if(fn2 && fn2_flag) fprintf(fp,"-fn2\t\"%s\"\n",fn2);
	if(inc_flag) fprintf(fp,"-inc\t%d\n",inc);
	if(outpatt && outpatt_flag) fprintf(fp,"-outpatt\t\"%s\"\n",outpatt);
	if(patt && patt_flag) fprintf(fp,"-patt\t\"%s\"\n",patt);
	if(patt2 && patt2_flag) fprintf(fp,"-patt2\t\"%s\"\n",patt2);
	if(start_flag) fprintf(fp,"-start\t%d\n",start);
}

void
_puttCalc_parameter :: PrintHelp(  FILE *fp, const char *pn )
{
	fprintf(fp,"\n%s\n========================\n",pn);	fprintf(fp,"\nSyntax: %s [options]\n\n",pn);

	fputs("Valid options are:\n\n",fp);
	fputs("  -h		print this help\n",fp);
	fprintf(fp,"\nOptional parameters:\n");
	fprintf(fp,"  -%-6s file	camera file                                           (def:)\n","cam");
	fprintf(fp,"  -%-6s file	camera file pattern                                   (def:.)\n","campatt");
	fprintf(fp,"  -%-6s file	config file                                           (def:)\n","cfn");
	fprintf(fp,"  -%-6s int	Deinterlace video: 0=no, 1=field 1 up, 2=field 1+2 [0 .. 3]         (def:0)\n","deint");
	fprintf(fp,"  -%-6s int	image width for raw file formats     [0 .. 1000000]   (def:704)\n","dx");
	fprintf(fp,"  -%-6s int	image height for raw file formats    [0 .. 1000000]   (def:576)\n","dy");
	fprintf(fp,"  -%-6s int	end                                  [0 .. 1000000]   (def:0)\n","end");
	fprintf(fp,"  -%-6s int	filter for YUV conversion: 0=no, 1=2-tab, 2=4-tab, 3=6-tab [0 .. 3]         (def:0)\n","filt");
	fprintf(fp,"  -%-6s int	file format (1= raw (RGB), 2= raw (BGR), 3= YCbCr) [0 .. 3]         (def:2)\n","fmt");
	fprintf(fp,"  -%-6s file	file name                                             (def:.)\n","fn");
	fprintf(fp,"  -%-6s file	file name2                                            (def:.)\n","fn2");
	fprintf(fp,"  -%-6s int	increment                            [1 .. 1000000]   (def:1)\n","inc");
	fprintf(fp,"  -%-6s file	output file pattern                                   (def:.)\n","outpatt");
	fprintf(fp,"  -%-6s file	file pattern                                          (def:.)\n","patt");
	fprintf(fp,"  -%-6s file	file pattern2                                         (def:.)\n","patt2");
	fprintf(fp,"  -%-6s int	start                                [0 .. 1000000]   (def:0)\n","start");
}


int ParseArg_puttCalc (int argc, char *argv[],_puttCalc_parameter  & para )
{
	int     i;  FILE    *fp;

	int errcount(0);
	for(i=1; i<argc; ) {
		const int retval = para.ParseArg(  argc-i, argv+i );
		if (retval==0) {
			cerr<<"ERROR: Failed to read in argument #"<<i<<": argv["<<i<<"]="<<argv[i]<<endl;
			++errcount;
			break;
		}
		i += retval;
	}
	if(para.help_flg) return 0;
	if(errcount>0) return -errcount;
	//
	//      second pass: read command line args and overwrite args
	//
	for(i=1; i<argc; ) {
		const int retval = para.ParseArg(  argc-i, argv+i );
		if (retval==0) {
			cerr<<"ERROR: Failed to read in argument #"<<i<<": argv["<<i<<"]="<<argv[i]<<endl;
			++errcount;
			break;
		}
		i += retval;
	}
	if(errcount>0) return -errcount;
	return 1;
}