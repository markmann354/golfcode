# VPlibs - Environment
# Version 1.0	Oliver Grau, May 2004
# Version 1.1	Peter Schubel, Feb 2012
#
# For debug compilation please compile the application with DEBUG specified. The
# preprocessor will use the specified DEBUG value in any C/C++ code.
# The compiled application name will be suffixed with 'D'. Use e.g.:
#	$ make DEBUG=2
#
# For profiling information please compile the application with PROF specified.
# To generate profiling info used by 'gprof' (execution will create gmon.out):
#	$ make PROF=1

# set NAME to the name of the executeable. Implies that you have $(NAME).cc
# file that contains your main procedure
NAME	:= puttCalc

# insert your local object files in the following list. There is a rule
# to translate them from a *.cc file. Example: LIBCXXOBJ	:= myfile1.o myfile2.o
LIBCXXOBJ	:= $(NAME)_parse.o
# Add local object files here, e.g.:
LIBCXXOBJ +=

## specify your external libraries here:
#USE_FFTW3=1
#USE_LEPTONLIB=1
#USE_MAGICK_API=1
#USE_OPENCV=1
#USE_OPENEXR=1
#USE_OPENMP=1
#USE_SIFT=1

## global config file
#include	$(VPLIBSHOME)/etc/Config.mk

##
## VPlibs module configuration. Uncomment the packages you need
## the config files extend the following variables:
##     VPLIBSFLGS, VPLIBSINCL, VPLIBSLIBS, VPLIBDSLIBS
## Modules that depend on other modules should be included before these dependencies!
#include $(VPLIBSHOME)/DVSlib/etc/Config.mk
#include $(VPLIBSHOME)/VPestimate/etc/Config.mk
#include $(VPLIBSHOME)/VPgl/etc/Config.mk
#include $(VPLIBSHOME)/VPtexturing/etc/Config.mk
#include $(VPLIBSHOME)/VPgeom/etc/Config.mk
#include $(VPLIBSHOME)/VPimageproc/etc/Config.mk
#include $(VPLIBSHOME)/VPip/etc/Config.mk
#include $(VPLIBSHOME)/VPutil/etc/Config.mk
#include $(VPLIBSHOME)/VPcoreio/etc/Config.mk
#include $(VPLIBSHOME)/VPbase/etc/Config.mk

# Finally include the sub-makefiles for external dependencies
#include $(VPLIBSHOME)/etc/Config_alloptional.mk

## add local definitions that you want to use for test purposes:
## Use the lines below to pick up the library and header files
## from your sandbox if you want to test headers & library before
## doing a full install!!
## (please do not commit changes here into the repository!)
LOCALFLGS	+= 
LOCALINCL	+= #-I/home/ograu/project/VPbase/include
LOCALLIBS	+= #/home/ograu/project/VPbase/src/libVPbase.a -ltiff -ljpeg

#include $(VPLIBSHOME)/etc/common_example.mk

