/*
 * File: PatchCorrespondenceGen.cc
 *
 * Copyright (c) 2007 The British Broadcasting Corporation.
 * <b>ALL RIGHTS RESERVED</b>.
 *
 * Copyright in the whole and every part of these source files belongs
 * to The British Broadcasting Corporation (BBC) and they
 * may not be used, sold, licenced, transferred, copied or reproduced
 * in whole or in part in any manner or form or in or on any media to
 * any person without the prior written consent of the BBC.
 */

#include <math.h>       /* sin, cos, sqrt */
#include <stdio.h>      /* printf */
#include <malloc.h>
#include <stdlib.h>  // needed for VC++
#include <iostream>
#include <string>
#include <string.h>
#include <fstream>
#include <sys/time.h>


using namespace std;

#include "PatchCorrespondenceGen.h"
#include "raf.h"
#include "nd_array.h"           // for alloc_nd


#include "RandomForest.h"


#define PI 3.14159265
#define RPD (PI/180.0)

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

bool debug = false;

using namespace bbcvp;

PatchCorrespondenceGen::PatchCorrespondenceGen(PoseEstPanTiltHead *PoseEst,
                      int patchLenX, int patchLenY,
                      int len_x, int len_y,
                      int spacingX, int spacingY, int nResolutions,
                      int maxPatches, int maxPatchesPerRegion,
                      int maxFixedWorldLines,
                      int forestSize, int maxdepth, int threshold, int degree, int motionMaskResolution)
{

    if (forestSize>0)  // TESTING: option not to build classifier as ElectricFence doesn't seem to be able to cope (at least, on GAT's laptop!)
    {

   rf = new randomForest(forestSize, degree, maxdepth, threshold, 0); //forestSize, degree, maxdepth, threshold, fuzzyP
   buildClassifier();


   freeMove = false;

   //TEMPORARY
   printMatch = 0.0;
    }  // TESTING: optionally don't build classifier


    //printf("PatchCorrespondenceGen::PatchCorrespondenceGen: patchLenX,Y=%d,%d  len_x,y=%d,%d  spacingX,Y=%d,%d nRes=%d  maxPatches=%d  maxPatchesPerRegion=%d  maxFixedWorldLines=%d\n",patchLenX, patchLenY, len_x, len_y, spacingX, spacingY, nResolutions, maxPatches, maxPatchesPerRegion, maxFixedWorldLines);


    m_PoseEst = PoseEst;
    m_patchLenX = patchLenX;
    m_patchLenY = patchLenY;
    m_lenX = len_x;
    m_lenY = len_y;

    m_nRegionsX = int ((double)m_lenX / (double)spacingX + 0.5);  if  (m_nRegionsX <= 0) m_nRegionsX=1;  // number of regions, rounded
    m_nRegionsY = int ((double)m_lenY / (double)spacingY + 0.5);  if  (m_nRegionsY <= 0) m_nRegionsY=1;  // with test to stop silly values

    // compute region spacing to ensure whole image is covered:
    m_spacingX = (m_lenX+m_nRegionsX-1) / m_nRegionsX;  // will round up spacing if needed to ensure whole image covered
    m_spacingY = (m_lenY+m_nRegionsY-1) / m_nRegionsY;
    printf("PatchCorrespondenceGen::PatchCorrespondenceGen: Image will be covered by %d x %d regions of dims %dx%d for feature-finding purposes\n",
      m_nRegionsX, m_nRegionsY, m_spacingX, m_spacingY);

    m_nResolutions = nResolutions;
    m_maxPatches = maxPatches;
    m_maxFixedWorldLines = maxFixedWorldLines;
    this->setFeatureDetectionIgnoreRegion(0, 0, 0, 0);

    // Default values, can be changed by a set method
    m_maxPatchesPerRegion = maxPatchesPerRegion;  // max number for createNewPatches() to create in one region
    m_maxPatchesPerRegionForDistanceCheck = m_maxPatchesPerRegion * 16;  // size of storage for projected patches - could be this much because....
    m_arrayLenForFeatureFind = m_lenX * m_lenY / 2;  // the absolute worst-case number of features that could be found

    //         ...region being tested could pick up patches from corners of 4 regions that might each have max no. of points in
    m_featureFindSsX = 1;  // step size when searching for good features in createNewPatches()
    m_featureFindSsY = 1;
    m_minFeatureVal = 0;         // threshold used in createNewPatches().
    m_minFeatureValBigEigen = 0; // threshold used in createNewPatches().
    m_halfWidX = 4;        // half-widths of feature detection window - default changed from 2 to 4 April 09
    m_halfWidY = 4;
    m_minDist  = 10;       // min distance in pixels between features when finding new ones
    // don't find features closer than half a block width from the border, so the lowest-level block lies within the picture:
    m_featureCreateBorderX = (patchLenX+1)/2;
    m_featureCreateBorderY = (patchLenY+1)/2;
    // initialise "pointers" for current, previous & previous-but-one images
    m_cur=0;
    m_prev=2;
    m_prevButOne=1;

    // Note how many times setImage() has been called - so we can check we have both cur & prev imagea
    m_nImagesLoaded = 0;
    m_firstGoodImage = 0;  // the image count when invalidateImage() was last called - used to prevent motion mask calc until enough images have passed

    // No texture patches stored yet
    m_nPatches = 0;

    m_image = new unsigned char**[3];
    // allocate storage for image pyramids
    for (int i=0; i<3; i++)  // for prev and current image
    {
   m_image[i]=new unsigned char*[m_nResolutions];
   int s=1;  // subsampling factor for the first level
   for (int r=0; r<m_nResolutions; r++)
   {
       m_image[i][r] = new unsigned char[m_lenX * m_lenY / s];
       int size = m_lenX * m_lenY / s;  //printf("Storage size for level %d = %d\n", r, size);
       s = s * 4;   // next level has half number of pixels in both x and y dir, so a factor of 4 smaller
   }
    }

    //printf("PatchCorrespondenceGen::PatchCorrespondenceGen: allocating space for %d patches and %d levels\n", maxPatches, m_nResolutions);
    m_incX = new int[m_nResolutions];
    m_incY = new int[m_nResolutions];

    m_prevPosX = new double[m_nResolutions];  // for holding pos of patch to look for at each level
    m_prevPosY = new double[m_nResolutions];
    m_texturePatches = new TexturePatch*[m_maxPatches];
    m_tempImage = new unsigned char[m_lenX * m_lenY / 2];  // for holding image after horiz-sumbsapling

    // create a new texture patch object for each patch
    // (used to do this each time a patch was made, but do it here so that any memory
    //  allocation problems are caught at the start, and so that patches can be deleted
    //  without needing to deallocate/reallocate)
    for (int i=0; i<m_maxPatches; i++)
   m_texturePatches[i] = new TexturePatch(m_patchLenX, m_patchLenY, m_nResolutions);

    // create 2D array to record how many patches are observed when trying to track
    // (used by AddAptchesFromImageUsingEstimatedPose to know which bits of the image to
    // look for new patches in)

    m_nPatchesSeen = (int**) alloc_nd(sizeof(int), 2, m_nRegionsX, m_nRegionsY);
    m_patchPosX = (int ***) alloc_nd(sizeof(int), 3, m_nRegionsX, m_nRegionsY, m_maxPatchesPerRegionForDistanceCheck);
    m_patchPosY = (int ***) alloc_nd(sizeof(int), 3, m_nRegionsX, m_nRegionsY, m_maxPatchesPerRegionForDistanceCheck );
    m_patchIndex = (int ***) alloc_nd(sizeof(int), 3, m_nRegionsX, m_nRegionsY, m_maxPatchesPerRegionForDistanceCheck );
    m_notePatchWhenLearning = (bool ***) alloc_nd(sizeof(bool), 3, m_nRegionsX, m_nRegionsY, m_maxPatchesPerRegionForDistanceCheck );
    m_usefulness = new double[m_maxPatchesPerRegionForDistanceCheck];
    for (int x=0; x<m_nRegionsX; x++) for (int y=0; y<m_nRegionsY; y++) m_nPatchesSeen[x][y] = 0;

    // no triangle strips for ignore regions set yet
    // (can't just call setIgnoreTriStrips with zero triangles as it clears prev temp storage first, based on prev number of tri strips!)
    m_nTriStrips=0;
    m_ignoreWithinTriangles=true;  // triangles show ignore regions (rather than 'use' regions) - needed so that it works by default with no triangles
    m_useIgnoreTriStrips=true;     // enable use of tri strip ignore regions

    // storage for features found in createNewPatches():
    m_FeatureX = new int[m_arrayLenForFeatureFind];
    m_FeatureY = new int[m_arrayLenForFeatureFind];
    m_FeatureVal = new int[m_arrayLenForFeatureFind];

    // for initialisation:
    m_FeatureTempX = new int[m_maxPatches];  // TODO: probably replace 1000 with m_maxPatches? - check with Rob
    m_FeatureTempY = new int[m_maxPatches];
    m_FeatureTempVal = new int[m_maxPatches];


    match_array.resize(150);

    // initialise arrays & counter for searching regions of the image to find new features: these are used to
    // allow only a subset of regions to be searched on each call to createNewPatches()
    m_nextRegionToSearch = 0;  // the index of the next region to search
    m_regionToSearchX = new int[ m_nRegionsX * m_nRegionsY];
    m_regionToSearchY = new int[ m_nRegionsX * m_nRegionsY];
    for (int rx=0; rx<m_nRegionsX; rx++)
   for (int ry=0; ry<m_nRegionsY; ry++)
   {
       m_regionToSearchX[rx + ry * m_nRegionsX] = rx;
       m_regionToSearchY[rx + ry * m_nRegionsX] = ry;
   }
    // TODO: randomise the elements of  m_regionToSearchX/Y so that searching takes place in a random order

    // allocate space for arrays used in findCorrespondencies (so they don't have to be allocated on
    // the stack or get new/deleted every time)
    m_imX = new double[m_maxPatches];
    m_imY = new double[m_maxPatches];
    m_directionalWeight = (double**)alloc_nd(sizeof(double), 2, m_maxPatches, 4);
    m_kltResult = new double[m_maxPatches];
    m_patchVisible = new bool[m_maxPatches];

    m_KLTTracker = new KLTTracker();

    this->setIgnoreFeatureLines(0);  // no lines to ignore features on (yet)

    this->setVelocityRangeForNoRelearning(1.0e20, 1.0e20);  // consider all features, regardless of velocity, when checking closeness
    this->setFocalLengthRatioForNoRelearning(1.0e20);       // consider all features, regardless of focal length when checking closeness
    this->setMaxPatchesToSearchPerRegion(9999999);          // search all patch es we can see by default
    this->setPatchPrefWeights(0.1, 0.5);  // weight drops by 1.0 for a 10 pix/fp speed diff or a 2:1 scale change
    this->setFeatureFindLevel(0);         // createNewPatches() operates on full-res image by default
    this->setMotionMaskComputationAcrossFrame(true);  // motion mask is computed across a frame period
    this->setMaxFractionToDiscardUsingMotionMask(0.5);  // motion mask cannot throw out more than this fraction of the visible patches

    // allocate storage for lines to ignore features near
    if (m_maxFixedWorldLines > 0) m_linesToIgnore = new FittedLine[maxFixedWorldLines];
    // create array for motion mask
    m_gotValidCamMotionMask = false;
    if(motionMaskResolution<0 || motionMaskResolution>=nResolutions) {
        m_motionMaskResolution = -1;
        m_motionMask = NULL;
        m_motionMaskReverse = NULL;
        m_interFrameMappingX = NULL;
        m_interFrameMappingY = NULL;
    } else {
      //int ssFactor = 1 << motionMaskResolution;
      //int arraySize = len_x*len_y*ssFactor*ssFactor;
      // above code looks like it makes bigger arrays as higher (and thus smaller) levels are used - try this instead:
      m_motionMaskResolution = motionMaskResolution;
      int arraySize =  (m_lenX >> m_motionMaskResolution) * (m_lenY >> m_motionMaskResolution);
      printf("PatchCorrespondenceGen::PatchCorrespondenceGen: allocating arrays of length %d for motion mask stuff\n", arraySize);
      m_motionMask = new bool[arraySize];
      m_motionMaskReverse = new bool[arraySize];
      m_interFrameMappingX = new double[arraySize]; // need arrays for x and y coords so double size with bit shift
      m_interFrameMappingY = new double[arraySize];
    }
}

PatchCorrespondenceGen::~PatchCorrespondenceGen()
{
    //printf("PatchCorrespondenceGen::~PatchCorrespondenceGen...\n");
    delete m_KLTTracker;
    delete m_patchVisible;
    delete m_kltResult;
    free_nd(m_directionalWeight, 2);

    delete m_imY;
    delete m_imX;
    delete m_prevPosX;
    delete m_prevPosY;
    delete m_incY;
    delete m_incX;
    delete m_tempImage;
    delete m_regionToSearchX;
    delete m_regionToSearchY;
    if (m_maxFixedWorldLines > 0) delete[] m_linesToIgnore;

    //printf("PatchCorrespondenceGen::~PatchCorrespondenceGen: deleting m_image...\n");
    for (int i=0; i<2; i++)  // for prev and current image
    {
   for (int r=0; r<m_nResolutions; r++)
       delete m_image[i][r];
   delete m_image[i];
    }
    delete m_image;

    //printf("PatchCorrespondenceGen::~PatchCorrespondenceGen: deleting m_texturePatches...\n");

    for (int i=0; i< m_maxPatches; i++) delete m_texturePatches[i];
    delete m_texturePatches;

    free_nd(m_nPatchesSeen, 2);
    free_nd(m_patchPosX, 3);
    free_nd(m_patchPosY, 3);
    free_nd(m_patchIndex, 3);
    free_nd(m_notePatchWhenLearning, 3);
    delete m_usefulness;
    delete m_FeatureX;
    delete m_FeatureY;
    delete m_FeatureVal;
    delete m_FeatureTempX;
    delete m_FeatureTempY;
    delete m_FeatureTempVal;

    // clear any storage already allocated for triangle strips
    if (m_nTriStrips != 0)
    {
   free_nd(m_projTri, 2);
   free_nd(m_projTriPointUsable, 2);

        for( int n = 0; n < m_nTriStrips; n++ ) {
          delete m_triPoints[n];
        };
        delete m_triPoints;
        delete m_nTriPoints;
    }

    if (m_motionMaskResolution >= 0)
      {
   delete[] m_motionMask;
   delete[] m_motionMaskReverse;
   delete[] m_interFrameMappingX;
   delete[] m_interFrameMappingY;
      }
    //printf("PatchCorrespondenceGen::~PatchCorrespondenceGen completed\n");

}

void PatchCorrespondenceGen::loadImage(unsigned char *r, unsigned char *g, unsigned char *b,
                   int inc_x, int inc_y,
                   int weightR, int weightG, int weightB)
{

    //-----image copies for classifier
//    m_r = r;
//    m_g = g;
//    m_b = b;
//    id = new imageDetails(m_r, m_g, m_b, m_lenX, m_lenY, inc_x, inc_y);

    int filter_ap=5;   // used 5 as the filter size by default pretty much since the start of the work
    //int filter_ap=7;  // was trying other values

    // increment points and wrap around when got to end of buffer (0,1,2)
    m_cur++; if (m_cur>2) m_cur=0;
    m_prev++; if (m_prev>2) m_prev=0;
    m_prevButOne++; if (m_prevButOne>2) m_prevButOne=0;

    for (int y=0; y<m_lenY; y++)
    {
   // for each line of incoming image, compute single-component value, store in top-level image
   unsigned char *lumOut = m_image[m_cur][0] + y * m_lenX;  // write result into top level
   unsigned char *rIn = r + y * inc_y;
   unsigned char *gIn = g + y * inc_y;
   unsigned char *bIn = b + y * inc_y;

   if (weightR == 255)
       for (int x=0; x<m_lenX; x++)
       {
      *lumOut++ = *rIn;
      rIn += inc_x;
       }
   else if (weightG == 255)
       for (int x=0; x<m_lenX; x++)
       {
      *lumOut++ = *gIn;
      gIn += inc_x;
       }
   else if (weightB == 255)
       for (int x=0; x<m_lenX; x++)
       {
      *lumOut++ = *bIn;
      bIn += inc_x;
       }
   else
       for (int x=0; x<m_lenX; x++)
       {
      int lum = *rIn * weightR + *gIn * weightG + *bIn * weightB;
      *lumOut++ = lum / 255;
      rIn += inc_x;
      gIn += inc_x;
      bIn += inc_x;
       }

   // then filter/subsample line straight away & put into temp image (to do horiz downsampling for 2nd stage, if
   // there is one)
   if (m_nResolutions > 1)
   {
       int lenXTo = m_lenX/2;
       raf_ss_uchar(m_image[m_cur][0]+ y * m_lenX,  // read from current line of single-comp image we've just made
          m_tempImage      + y * lenXTo,  // write to horizontally-subsampled image
          1, 1,                           // spacing of samples in both input and output images is 1
          m_lenX,                         // number of samples to read in
          filter_ap,                      // aperture of filter
          2);                             // subsampling factor is 2
   }

    }  // loop for y doing RGB->Y conversion and 2:1 horiz downsampling

    // do vertical filter/subsample for 2nd level image if we have more than 1 level
    if (m_nResolutions > 1)
    {
   for (int x=0; x<m_lenX/2; x++)
   {
       raf_ss_uchar(m_tempImage + x,          // read from the top of the x th column in the already-downsampled image
          m_image[m_cur][1] + x,    // write to 2nd level image, column x
          m_lenX/2, m_lenX/2,       // in & out spacing is line length in 2x downsampled image
          m_lenY,                   // number of samples to read in
          filter_ap,                      // aperture of filter
          2);                       // downsample factor
   }
    }


    // Then to horiz & vert downsamples for subsequent levels (levels 0 & 1 are already done now)
    int lenXFrom = m_lenX/2;  // dims of level we read from
    int lenYFrom = m_lenY/2;
    for (int l=2; l<m_nResolutions; l++)
    {
   int lenXTo = lenXFrom/2;
   int lenYTo = lenYFrom/2;
   //printf("PatchCorrespondenceGen::loadImage: writing into level %d with dims %d x %d, total length %d\n",
   //     l, lenXTo, lenYTo, lenXTo*lenYTo);

   for (int y=0; y<lenYFrom; y++)                     // for each row at the ionput resolution, do a horiz downsample:
       raf_ss_uchar(m_image[m_cur][l-1] + y * lenXFrom, // read from current line of single-comp image we've just made
          m_tempImage  + y * lenXTo,          // write to horizontally-compressed temp image
          1, 1,                               // spacing of samples in both input and output images is 1
          lenXFrom,                           // number of samples to read in
          filter_ap,                      // aperture of filter
          2);                                 // subsampling factor is 2
   //printf("PatchCorrespondenceGen::loadImage: downfiltering each column...\n");

   for (int x=0; x<lenXTo; x++)                       // for each column in subsampled image, do a vertical subsample
       raf_ss_uchar(m_tempImage + x,                    // read from the top of the x th column in the already-downsampled image
          m_image[m_cur][l]  + x,             // write to next level image, column x
          lenXTo, lenXTo,                     // in & out spacing is line length in downsampled image
          lenYFrom,                           // number of samples to read in
          filter_ap,                      // aperture of filter
          2);                                 // downsample factor
   //printf("PatchCorrespondenceGen::loadImage: finished this level\n");

   lenXFrom = lenXTo;  // for next loop
   lenYFrom = lenYTo;
    }

    // set m_IncX[] and m_IncY to the appropriate values for each level
    // as these are needed by the KLT tracker class
    int lx = m_lenX;
    for (int l=0; l<m_nResolutions; l++)
    {
   m_incX[l] = 1;     // all have samples close-packed horizontally
   m_incY[l] = lx;    // vertical spacing is equal to line length, that halves each level
   lx = lx/2;
    }

    // note that another image has been loaded
    m_nImagesLoaded++;


    id = new imageDetails(m_image[0][0], m_lenX, m_lenY, m_incX[0], m_incY[0]);


}

unsigned char** PatchCorrespondenceGen::getImagePointers(int image)
{
  if (image == 0)
    return m_image[m_cur];
  else if (image==1)
    return m_image[m_prev];
  else
    return m_image[m_prevButOne];
}



void PatchCorrespondenceGen::setIgnoreFeatureLines(double ignoreDistFromLine)
{
    m_ignoreDistFromLineSquared = ignoreDistFromLine * ignoreDistFromLine;
}


void PatchCorrespondenceGen::setIgnoreTriStrips(int nTriStrips, int *nTriPoints, double **triPoints, bool ignoreWithinTriangles)
{
    // clear any storage already allocated for triangle strips
    if (m_nTriStrips != 0)
    {
   free_nd(m_projTri, 2);
   free_nd(m_projTriPointUsable, 2);

        for( int n = 0; n < m_nTriStrips; n++ ) {
          delete m_triPoints[n];
        };
        delete m_triPoints;
        delete m_nTriPoints;
    }

    m_nTriStrips=nTriStrips;
    m_nTriPoints=new int[nTriStrips];
    m_triPoints= new double*[nTriStrips];
    m_ignoreWithinTriangles=ignoreWithinTriangles;

    // Make a copy of incoming triangle strips
    for( int n = 0; n < m_nTriStrips; n++ ) {
      m_nTriPoints[n] = nTriPoints[n];
      m_triPoints[n] = new double[m_nTriPoints[n]*3];
      for ( int i = 0; i < m_nTriPoints[n]; i++ ) {
        m_triPoints[n][3*i+0] = triPoints[n][3*i+0];
        m_triPoints[n][3*i+1] = triPoints[n][3*i+1];
        m_triPoints[n][3*i+2] = triPoints[n][3*i+2];
      }
    };

    // allocate storage for use when projecting vertices into the image
    // first, find the max number of points in each triangle strip, to determine stoarge space
    int maxPoints=0;
    for (int n=0; n<m_nTriStrips; n++) if (m_nTriPoints[n] > maxPoints) maxPoints=m_nTriPoints[n];

    m_projTri = (double **) alloc_nd(sizeof(double), 2, m_nTriStrips, maxPoints*2);  // for each tri strip, need 2 locs for each vertex (x & y im coords)
    m_projTriPointUsable = (bool **)alloc_nd(sizeof(bool), 2, m_nTriStrips, maxPoints);

}


bool PatchCorrespondenceGen::createNewPatch(int imageX, int imageY,
                   double worldX, double worldY, double worldZ,
                   int fixedAxis, int expiryTime, double patchVelX, double patchVelY)
{

    if (m_nPatches >= m_maxPatches)
    {
   cout << "PatchCorrespondenceGen::createNewPatch: exceeded number of allowed paches!" << endl;
   return (false);
    }
    else
    {
   //printf("PatchCorrespondenceGen::createNewPatch: creating new patch %d with im coords %d,%d, woorld coords %6.2f,%6.2f,%6.2f and velocity %6.2f,%6.2f\n",
   //       m_nPatches, imageX, imageY, worldX, worldY, worldZ, patchVelX, patchVelY);

   // (NB no need to create a new texture patch object for this patch
   // as they were all allocated in the constructor)

   m_texturePatches[m_nPatches]->setImageAllLevels(m_nResolutions,
                     m_image[m_cur],
                     m_incY,
                     m_lenX, m_lenY,
                     (double)imageX, (double)imageY);  // patch pos is an int at top level, but texture patch obj can cope with floats

   // m_texturePatches[m_nPatches]->setPos(worldX, worldY, worldZ);  // don't store 3D position in texture patch any more
   m_texturePatches[m_nPatches]->setFixedAxis(fixedAxis);
   m_texturePatches[m_nPatches]->setExpiryTime(expiryTime);
   m_texturePatches[m_nPatches]->setVelocity(patchVelX, patchVelY);
   m_texturePatches[m_nPatches]->setTimesSeen(0);
   m_texturePatches[m_nPatches]->setTimesInlier(0);
   double focLen;
   m_PoseEst->getCamFocalLength(&focLen);
   m_texturePatches[m_nPatches]->setFocalLength(focLen);


   // add the world patch coords to the PoseEstPanTiltHead object
   // NB point now added to object with ID 1 greater than patch index (ie. one point per object), leaving object 0 for points
   // that define the world ref frame
   bool fixPosX=true;
   bool fixPosY=true;
   bool fixPosZ=true;
   if (fixedAxis==1) {fixPosY=false; fixPosZ=false; }
   if (fixedAxis==2) {fixPosZ=false; fixPosX=false; }
   if (fixedAxis==3) {fixPosX=false; fixPosY=false; }
   int pointIndex = m_PoseEst->addPoint(worldX, worldY, worldZ, m_nPatches, fixPosX, fixPosY, fixPosZ); // pos w.r.t. camera at origin
   if (pointIndex >=0)
   {
       m_nPatches++;
       //printf("PatchCorrespondenceGen::createNewPatch: RETURNING TRUE\n");
       return (true);
   }
   else
   {
       //printf("PatchCorrespondenceGen::createNewPatch: RETURNING FALSE\n");
       return (false);
   }

    }  // if not exceeded number of patches

} // end of createNewPatch method

bool PatchCorrespondenceGen::addNewTextureToPatch(int patchID, double imageX, double imageY)
{
    cout << "PatchCorrespondenceGen::AddNewTextureToPatch not implemented yet!" << endl;
    exit(-1);
}

/*
 *
 * PointInTriangle - local function used in createNewPatches
 *
 * Calculates whether point (Px,Py) is inside triangle (A,B,C)
 *
 * @param Px x coordinate of test point
 * @param Py y coordinate of test point
 * @param A  triangle vertex A where A[0] = Ax and A[1] = Ay
 * @param B  triangle vertex B where B[0] = Bx and B[1] = By
 * @param C  triangle vertex C where C[0] = Cx and C[1] = Cy
 *
 * @return whether point is inside triangle or not
 *
 */
bool PatchCorrespondenceGen::pointInTriangle( double Px, double Py, double *A, double *B, double *C ) {

//
// Calculate the barycentric coordinates u,v of P in Triangle ABC
// If P is inside ABC, u,v > 0 and u + v < 1
//
//
// See : http://www.blackpawn.com/texts/pointinpoly/default.html
//
//
    double v0x = C[0] - A[0];
    double v0y = C[1] - A[1];

    double v1x = B[0] - A[0];
    double v1y = B[1] - A[1];

    double v2x = Px - A[0];
    double v2y = Py - A[1];

    double dot00 = v0x*v0x + v0y*v0y;
    double dot01 = v0x*v1x + v0y*v1y;
    double dot02 = v0x*v2x + v0y*v2y;
    double dot11 = v1x*v1x + v1y*v1y;
    double dot12 = v1x*v2x + v1y*v2y;

    double invDenom = 1.0 / ( dot00 * dot11 - dot01 * dot01 );
    double u = ( dot11 * dot02 - dot01 * dot12 ) * invDenom;
    double v = ( dot00 * dot12 - dot01 * dot02 ) * invDenom;

    return ( u > 0 ) && ( v > 0 ) && ( ( u + v ) < 1 );
}


int PatchCorrespondenceGen::createNewPatches(int minPatchesPerRegion, int nRegionsToSearch, int expiryTime)
{

    // if nRegionsToSearch is negative, this means search them all
    if (nRegionsToSearch < 0) nRegionsToSearch = m_nRegionsX * m_nRegionsY;
    if (nRegionsToSearch > m_nRegionsX * m_nRegionsY) nRegionsToSearch = m_nRegionsX * m_nRegionsY;  // catch too many being asked for

    //printf("PatchCorrespondenceGen::createNewPatches starting: nRegionsToSearch=%d m_VelocityRangeForNoRelearningY=%f,%f minPatchesPerRegion=%d m_maxPatchesPerRegion=%d m_minFeatureVal=%d m_minFeatureValBigEigen=%d  m_minDist=%d\n",
    //nRegionsToSearch, m_VelocityRangeForNoRelearningX, m_VelocityRangeForNoRelearningY, minPatchesPerRegion, m_maxPatchesPerRegion, m_minFeatureVal, m_minFeatureValBigEigen,  m_minDist);

    // before creating new patches, make a note of where any lines we're using to track from project to, and tell
    // the createNewPatches() method to ignore points that lie within a given distance of where these project to
    int nLinesVisible = 0;
    if (m_maxFixedWorldLines > 0 && m_ignoreDistFromLineSquared > 0.0)
    {
   for (int l=0; l<MIN(m_PoseEst->getNLines(), m_maxFixedWorldLines); l++)
   {
       double x0, y0, x1, y1;
       if (m_PoseEst->projectIntoImage(l, &x0, &y0, &x1, &y1) )  // projects for latest camera pose and reference object by default
       {
      m_linesToIgnore[nLinesVisible].clear();
      m_linesToIgnore[nLinesVisible].addPoint(x0, y0, false);
      m_linesToIgnore[nLinesVisible].addPoint(x1, y1, true);  // recalculate line after anning 2nd point
      nLinesVisible++;
       }
   } // for each line
    }

    bool doFullClosenessCheck = false;  // TEST: try this to see if it speeds up patch finding
    //bool doFullClosenessCheck = true;
    // For each "region" of the image that has insufficient patches found
    // in it when findCorrespondences() was last called....

    int newPatchesCreated = 0;

    // note the current image velocity (in pixels/lines)
    double curVelX, curVelY;
    m_PoseEst->getCurrentImageVelocity(&curVelX, &curVelY);
    //printf("PatchCorrespondenceGen::createNewPatches: current image velocity= %f %f\n", curVelX, curVelY);

    // optionally, compute existing patch positions using current pose (rather than usng ones left over from
    // an earlier call to findGorrespondencies(), and note the actual coodiantes of patches in each region
    // Note: here we ignore temp patches if looking for permanent ones; if we don't do the full closeness check then we use
    // the results from the projection carried out by the most recent call to findCorrespondencies() which will be ALL patches.
    // Thus both temp and perm patches will inhibit patch finding, so may not be what you want when creating perm patches if
    // temporary ones also exist.
    if (doFullClosenessCheck)
    {
   bool projectWithDistortion=true;  // whether to add distortion when projecting points, when working out where to look
   //printf("Doing full closeness check for all %d patches...\n", m_nPatches);
   // clear the counts of the number of points seen in each "region"
   for (int x=0; x<m_nRegionsX; x++) for (int y=0; y<m_nRegionsY; y++) m_nPatchesSeen[x][y] = 0;

   // project each patch into the camera using the PoseEst object - this gives the
   // location of each patch in the previous image.  Store the patch positions in an array.
   for (int i=0; i<m_nPatches; i++)
   {
       double imX, imY;
       double patchVelX, patchVelY;
       m_texturePatches[i]->getVelocity(&patchVelX, &patchVelY);
       //printf("Patch %d has velocity %f,%f\n", i, patchVelX, patchVelY);
       // if we are creating permanent patches, don't discard based on close temporary ones - only look at other perm ones
       // and also don't count patches at a significantly different velocity to the current velocity
       if ( ((expiryTime < 0 &&  m_texturePatches[i]->getExpiryTime() < 0) || expiryTime > 0) &&
       fabs(curVelX - patchVelX) < m_VelocityRangeForNoRelearningX &&
       fabs(curVelY - patchVelY) < m_VelocityRangeForNoRelearningY  )
       {
      if (m_PoseEst->projectIntoImageObjectPoint(0, &imX, &imY, projectWithDistortion, i))  // ith patch is 0th point on object i (optional distortion)
      {
          int rx = (int)imX / m_spacingX;
          int ry = (int)imY / m_spacingY;

          //printf("PatchCorrespondenceGen::createNewPatches: point %5.1f,%5.1f fell in region %d,%d at position %d\n",
          //   imX, imY, rx, ry, m_nPatchesSeen[rx][ry]);

          if (rx<0 || ry < 0 || rx > m_nRegionsX-1 || ry > m_nRegionsY-1)  // safety check.....
          { printf("PatchCorrespondenceGen::createNewPatches: point fell in region %d,%d which is outside image!!  im coords=%6.2f,%6.2f\n", rx, ry, imX, imY);
          exit (-1) ; }

          if ( m_nPatchesSeen[rx][ry] < m_maxPatchesPerRegionForDistanceCheck) // if space to store this one.....
          {
         if (rx<0 || ry < 0 || rx > m_nRegionsX-1 || ry > m_nRegionsY-1)
         { printf("PatchCorrespondenceGen::createNewPatches: point fell in region %d,%d which is outside image!!\n", rx, ry);
         exit (-1) ; }
         m_patchPosX[rx][ry][m_nPatchesSeen[rx][ry]] = int(imX+0.5);
         m_patchPosY[rx][ry][m_nPatchesSeen[rx][ry]] = int(imY+0.5);
         m_notePatchWhenLearning[rx][ry][m_nPatchesSeen[rx][ry]] = true;  // any patch we see here should be taken account of (unlike those left from findCorrespondences()).
         m_nPatchesSeen[rx][ry]++;
          }  // if space to store it
      }
      //else printf("PatchCorrespondenceGen::createNewPatches point %d fell outside image in full closeness check\n", i);
       }  // if expiry times are such that we care about these patches being close
   }  // loop for projecting each patch
    }  // if doing full closeness check, ie. recomputing patchPosX/Y


    if ( m_useIgnoreTriStrips )
      {
   // Project any triangle strip verticess into the image so we can test for new features falling in these:
   for (int n=0; n<m_nTriStrips; n++)
     {
       for (int i=0; i<m_nTriPoints[n]; i++)
         {
      // project this point into the image, allowing it to land outside but checking for it being behind the camera
      m_projTriPointUsable[n][i] =
        m_PoseEst->projectIntoImageArbitraryPoint(m_triPoints[n][3*i], m_triPoints[n][3*i+1], m_triPoints[n][3*i+2],
                         &m_projTri[n][2*i], &m_projTri[n][2*i+1],
                         false,  // don't bother about applying distortion, esp as could give weird results for points landing way outside the image
                         -1, -1, false);  // default object & image, don't trap points outside image
         }  // i=point in tri
     }  // loop for n=0..m_nTriPoints-1
      }

    // Loop for the max number of regions that we want to search this time:
    int regionsChecked = 0;  // number of regions that we have checked to see if we need to create features in
    int regionsSearched = 0; // number of regions that we have actually created features in
    int nFeaturesForMotionMaskTesting = 0;  // number of features that make it through to testing against motion mask: used to check it doesn't discard too many
    int nFeaturesDiscardedByMotionMask = 0; // number of features that were discarded by motion mask

    while (regionsChecked <  m_nRegionsX * m_nRegionsY && regionsSearched < nRegionsToSearch)
    {
   regionsChecked++;
   int rx = m_regionToSearchX[m_nextRegionToSearch];
   int ry = m_regionToSearchY[m_nextRegionToSearch];

   //printf("PatchCorrespondenceGen::createNewPatches: checked region %d,%d and found it had %d patches projecting into it\n", rx, ry, m_nPatchesSeen[rx][ry]);

   // count number of patches in this block that we should consider when deciding wither to create new patches or not
   // (as the number stored in m_nPatchesSeen[rx][ry] is the total
   int nPatchesSeenForConsideringDuringLearning = 0;
   for (int i=0; i<m_nPatchesSeen[rx][ry]; i++)
       if (m_notePatchWhenLearning[rx][ry][i]) nPatchesSeenForConsideringDuringLearning++;

   if ( nPatchesSeenForConsideringDuringLearning < minPatchesPerRegion)
   {
       // for each pixel in this region (as long as it is far enough away from the image edge), evaluate
       // the "goodness" of it in terms of being a trackable feature
       //int maxFeaturesToFind = m_maxPatchesPerRegion - nPatchesSeenForConsideringDuringLearning;
       int maxFeaturesToFind = m_maxPatchesPerRegion;  // always look for the max we need - we may 're-find' ones that we already have

       // work out the min & max coords we can serach within, noting that we can go outside the current
       // region as long as the aperture we'll use won't fall outside the image
       // (previously try extending by halfWid+2, the "halfWid" for the filter aperture, 1 for the gradient calc and 1 for local max check,
       // but given calc for usefulStart/Len/X/Y in m_KLTTracker->findGoodFeatures, we need '+1' and '-1' to ensure no overlap between
       // pixels that could receive maxima in adjacent blocks).

       // first, work out how close to the edge of the image we should look so as not to find features in the border region
       // (that will usually have been set to half a block width at the lowest scale)
       int additionalBorderX = m_featureCreateBorderX - m_halfWidX - 1; if (additionalBorderX < 0) additionalBorderX=0;
       int additionalBorderY = m_featureCreateBorderY - m_halfWidY - 1; if (additionalBorderY < 0) additionalBorderY=0;

       int xMin = MAX(m_spacingX*rx-m_halfWidX-1, additionalBorderX);
       int yMin = MAX(m_spacingY*ry-m_halfWidY-1, additionalBorderY);
       int xMax = MIN(m_lenX-additionalBorderX, m_spacingX*(rx+1)+m_halfWidX+1)-1;
       int yMax = MIN(m_lenY-additionalBorderY, m_spacingY*(ry+1)+m_halfWidY+1)-1;

       //printf("PatchCorrespondenceGen::createNewPatches: region %d,%d: searching region (%d,%d) to (%d,%d) which had %d prior relevant patches\n", rx, ry, xMin, yMin, xMax, yMax, nPatchesSeenForConsideringDuringLearning);

       // if all four corners lie in the 'ignore' region, we don't need to do the search:
       // (only check rectangular ignore region, not the triangle strips, as not easy to test whether whole of region lies within triangles)
       if (xMin >= m_minIgnoreX && xMin < m_maxIgnoreX && xMax >= m_minIgnoreX && xMax < m_maxIgnoreX &&
      yMin >= m_minIgnoreY && yMin < m_maxIgnoreY && yMax >= m_minIgnoreY && yMax < m_maxIgnoreY)
       {
      // printf("block %d,%d being totally ignored!\n", rx, ry);
       }
       else
       {
      regionsSearched++;
      int scale=1<<m_featureFindLevel;
      int xm=xMin/scale; int ym=yMin/scale;

      int featuresFound = m_KLTTracker->findGoodFeatures(
          m_image[m_cur][m_featureFindLevel] + xm * m_incX[m_featureFindLevel] + ym * m_incY[m_featureFindLevel],   // top-left pixel of region to look in
          (xMax-xMin+1)/scale, (yMax-yMin+1)/scale,         // dims of region to search
          m_incX[m_featureFindLevel], m_incY[m_featureFindLevel],         // arrays giving x & y increments for each level (current image)
          m_halfWidX, m_halfWidY,             // width around each pixel to sum (so ap is 2*halfWid+1 in both x & y)
          m_featureFindSsX, m_featureFindSsY, // step size when searching for maxima
	  m_arrayLenForFeatureFind,             // length of arrays available for use- will limit number to maxNewFeaturesToFind later
          m_FeatureX, m_FeatureY, m_FeatureVal,          // arrays to put coords of up to maxFeaturesToFind in
          m_minFeatureVal,
          m_minFeatureValBigEigen,
          m_minDist/scale);

      if (featuresFound > maxFeaturesToFind) featuresFound=maxFeaturesToFind;  // limit to the number we actually want

      //printf("PatchCorrespondenceGen::createNewPatches: region %2d,%2d: found %d features (max no=%d, min no.=%d minFeatureVal=%d\n",
      //rx, ry, featuresFound, <maxFeaturesToFind, minPatchesPerRegion, m_minFeatureVal);

      // add the features that have been found
      for (int i=0; i<featuresFound; i++)
      {
          // work out the coordinates of the feature in the whole image
          //printf("   region top-left=%d,%d   offset into this for feature=%d,%d\n", xMin, yMin, m_FeatureX[i], m_FeatureY[i]);
          int fx = m_FeatureX[i]*scale + xMin;
          int fy = m_FeatureY[i]*scale + yMin;
          //printf("   Feature coords are %d,%d and featureVal=%d\n", fx, fy, m_FeatureVal[i]);

          // first, check we're not in the discard region
          bool discard = (fx >= m_minIgnoreX && fx < m_maxIgnoreX &&
                fy >= m_minIgnoreY && fy < m_maxIgnoreY);

                    if ( m_useIgnoreTriStrips )
            {
         // check point does (not) lie within the region(s) defined by the triangle strips
         bool inTriangle=false;
         //printf("Checking for candidate feature %d, %d being inside trialgles...\n", fx, fy);
         for (int n=0; n<m_nTriStrips && !inTriangle && !discard; n++)
           {
             for (int i=2; i<m_nTriPoints[n] && !inTriangle; i++)  // start with the 3rd point in the list and consider triangle from this and two previous points
               {
            // if this vertex and previous two were all in front of the camera, consider this triangle:
            //printf("  Tri strip %d, starting with point %d:\n", n, i);
            if (m_projTriPointUsable[n][i] &&  m_projTriPointUsable[n][i-1] &&  m_projTriPointUsable[n][i-2])
              {
                //printf("    im coords of triangle: %5.1f %5.1f  %5.1f %5.1f  %5.1f %5.1f\n",
                //       m_projTri[n][2*i], m_projTri[n][2*i+1],
                //       m_projTri[n][2*(i-1)], m_projTri[n][2*(i-1)+1],
                //       m_projTri[n][2*(i-2)], m_projTri[n][2*(i-2)+1]);
                inTriangle = pointInTriangle((double)fx, (double)fy, &m_projTri[n][2*i], &m_projTri[n][2*(i-1)],&m_projTri[n][2*(i-2)]);
                //if (inTriangle) printf("    point inside\n"); else printf("     point outside\n");
              }
            //else printf("    One or more vertices was behind camera, so triangle not usable\n");
               }
           }
         // discard point if already decided to discard, or if within discard triangles or not in 'only within triangles'
         discard = discard || ( (inTriangle && m_ignoreWithinTriangles) || (!inTriangle && !m_ignoreWithinTriangles));
            }
          // next, check whether point lies on a line that we don;t want to find features on
          for (int i=0; i<nLinesVisible && !discard; i++)
          {
         if (m_linesToIgnore[i].getSquaredDistanceToPoint(fx, fy) < m_ignoreDistFromLineSquared) discard=true;
         //if (discard) printf("PatchCorrespondenceGen::createNewPatches: discarding feature as it is a dist %f from a line\n",m_linesToIgnore[i].getDistanceToPoint(fx, fy));
          }

          // next, check whether feature lies in an area flagged as foreground motion (using area based on equiv size at full-res,
          // taking account of feature find level)
          // check all points centred on patch position, going out to corners of patch at full resolution scale
          // any one of these being flagged as foreground motion will cause patch to be ignored
	  // NB: check for m_maxFractionToDiscardUsingMotionMask disabled (at least until a better way of doing it is found!), as currently
	  // it can stop discarding of unwanted features if most of the new ones found are in a motion-ignore region
          nFeaturesForMotionMaskTesting++;
          if (!discard && m_useMotionMaskAsIgnoreRegion && m_motionMaskResolution != -1 && m_gotValidCamMotionMask )
	    //&& (double)nFeaturesDiscardedByMotionMask / (double)nFeaturesForMotionMaskTesting <m_maxFractionToDiscardUsingMotionMask )
            {
	      //printf("Testing point %d,%d against motion mask at resolution %d...\n", fx, fy, m_motionMaskResolution);
         int masklenX = m_lenX >> m_motionMaskResolution;
         int masklenY = m_lenY >> m_motionMaskResolution;
         int dy = (((m_patchLenY<<m_featureFindLevel)-1)/2)>>m_motionMaskResolution; if (dy<1) dy=1;
         int starty = (fy>>m_motionMaskResolution) - dy;
         int endy   = (fy>>m_motionMaskResolution) + dy;
         int dx = (((m_patchLenX<<m_featureFindLevel)-1)/2)>>m_motionMaskResolution; if (dx<1) dx=1;
         int startx = (fx>>m_motionMaskResolution) - dx;
         int endx   = (fx>>m_motionMaskResolution) + dx;
         //printf("startx=%d endx=%d dx=%d\n", startx, endx, dx);
         //printf("starty=%d endy=%d dy=%d\n", starty, endy, dy);
         for (int yy =  starty; yy <= endy && !discard; yy++)
           {
             if (yy >= 0 && yy < masklenY)
               {
            bool *motionMask = m_motionMask + yy*masklenX;
            for (int xx =  startx; xx <= endx && !discard; xx++)
              {
                if (xx >= 0 && xx < masklenX)
                  {
               //printf("  Testing point %d,%d in motion mask: value=%d\n", xx, yy, (int)motionMask[xx]);
               discard = discard || motionMask[xx];  // discard if any tested pixel falls in mask area
               //if (motionMask[xx]) printf("createNewPatches: Ignored a feature at %d %d due to it falling in motion mask\n", fx, fy);
                  }
              }
               }
           }
         if (discard) nFeaturesDiscardedByMotionMask++;
            }
	  //else printf(" **** Skipped motion mask test as had already discarded %d out of %d tested and max fraction to discard is %6.2f\n", nFeaturesDiscardedByMotionMask, nFeaturesForMotionMaskTesting, m_maxFractionToDiscardUsingMotionMask);

          // check whether there was already a feature near here by looking in the current and 8-neighbouring blocks
          if (!discard)  // used to say && doFullClosenessCheck - took this out now that we record positions in findCorrespondences
          {
            //printf("Checking potentially new patch %d,%d for closeness to other patches...\n", fx, fy);
         for (int ry1=MAX(ry-1,0); ry1<=MIN(ry+1, m_nRegionsY-1) && !discard; ry1++)
         {
             for (int rx1=MAX(rx-1,0); rx1<=MIN(rx+1, m_nRegionsX-1) && !discard; rx1++)
             {
            for (int i1=0; i1<m_nPatchesSeen[rx1][ry1] && !discard; i1++)
            {
                if (m_notePatchWhenLearning[rx1][ry1][i1])  // only look at patches that are relevant (some may be in list but have inappropriate velocities)
                {
               int dx = fx-m_patchPosX[rx1][ry1][i1];
               int dy = fy-m_patchPosY[rx1][ry1][i1];
               //printf("...testing against patch %d in region %2d,%2d at im coords %3d,%3d.... dist is %d\n",
               //i1, rx1,ry1, m_patchPosX[rx1][ry1][i1], m_patchPosY[rx1][ry1][i1], (int)(sqrt(float(dx*dx+dy*dy))));
               if (dx*dx + dy*dy < m_minDist*m_minDist)
               {
                 //printf("...discarding patch at %d,%d as it is only %d away from another patch\n",
                 //     fx, fy, (int)(sqrt(float(dx*dx+dy*dy))));
                   discard = true;
               }
                }
            }
             }
         }
         //if (!discard) printf("...that patch is OK!\n");
          }  // if doing full closeness check

          if (!discard)
          {
         // create new feature: its 3D position must be specified w.r.t the origin of the reference object, since the position of
         // the new object gets set to this by poseEstPanTiltHead->addPoint().  Thus we subtract the position of the ref object, ie.
         // add the position of the camera
         // Get unit vector from camera in direction of this point
         double wx, wy, wz;
         double cx, cy, cz;
         //double pan, tilt, roll, fov;  // only needed for testing
         //m_PoseEst->getCamRotPTR(&pan, &tilt, &roll);
         //m_PoseEst->getCamFOV(&fov);
         m_PoseEst->lineOfSight((double)fx, (double)fy, &wx, &wy, &wz);  // unit vector from camera out of this pixel
         m_PoseEst->getCamPos(&cx, &cy, &cz);            // position of camera
         //printf("Line of sight = %6.2f,%6.2f,%6.2f    cam pos = %6.2f,%6.2f,%6.2f  cam ptr = %6.2f,%6.2f,%6.2f   fov=%6.2f\n", wx, wy, wz, cx, cy, cz, pan, tilt, roll, fov);

         int constrainedAxis = 0;
         double dist = 100;  // distance in m from camera to plane that point will be placed on
         double s;           // amount to scale unit vector by to place point on desired plane
         if (fabs(wx) > fabs(wy) && fabs(wx) > fabs(wz))  // fix point on plane at x=+ or - dist
         {
             s = dist / fabs(wx);
             constrainedAxis = 1;
         }
         else if (fabs(wy) > fabs(wx) && fabs(wy) > fabs(wz))  // fix point on plane at y=+ or - dist
         {
             s = dist / fabs(wy);
             constrainedAxis = 2;
         }
         else // fix point on plane at y=+ or - dist
         {
             s = dist / fabs(wz);
             constrainedAxis = 3;
         }
         // compute world coord to put point at

         wx = wx * s + cx;
         wy = wy * s + cy;
         wz = wz * s + cz;

         //printf("New feature created at based on im coords %3d %3d at world coords %6.2f,%6.2f,%6.2f after offseting from cam pos of %6.2f,%6.2f,%6.2f\n", fx, fy, wx, wy, wz, cx, cy, cz );
         if (createNewPatch(fx, fy, wx, wy, wz, constrainedAxis, expiryTime, curVelX, curVelY))  // if this succeeded......
             newPatchesCreated++;

          }   // if not too close to an existing feature

      }   // loop for i=features found in region
       }  // block non totally in a region to be ignored

   }      // if this region is short of patches

   // increment pointer to next region to searcfh, modulo the number of patches
   m_nextRegionToSearch++;
   if (m_nextRegionToSearch >=  m_nRegionsX * m_nRegionsY) m_nextRegionToSearch=0;

    } // loop for region counter for the number of regions to search this time

    return (newPatchesCreated);

}            // end of createNewPatches method

void PatchCorrespondenceGen::deleteAllPatches()
{
    m_nPatches=0;
    m_PoseEst->deleteNonRefPoints();
}

int PatchCorrespondenceGen::deleteOldPatches(int expiryTime)
{
    int deletedPatches = 0;
    for (int i=0; i<m_nPatches; i++)
    {
   if (m_texturePatches[i]->getExpiryTime() >= 0 && m_texturePatches[i]->getExpiryTime() <= expiryTime)
   {
       //printf("Deleting patch %d as its expiry time=%d and current time is %d...\n", i,
       // m_texturePatches[i]->getExpiryTime(), expiryTime );
       this->deletePatch(i);
       deletedPatches++;
       i--;  // so we go back and look at the one we've just moved into this slot
   }
    }
    return (deletedPatches);
}

int PatchCorrespondenceGen::deleteUnseenPatches()
{
    int deletedPatches = 0;
    for (int i=0; i<m_nPatches; i++)
    {
   if ( m_PoseEst->getNImagesWithObjectVisible(i) == 0)
   {
       //printf("Deleting patch %d as it is not visible in any stored camera\n", i);
       this->deletePatch(i);
       deletedPatches++;
       i--;  // so we go back and look at the one we've just moved into this slot
   }
    }
    return (deletedPatches);
}

int PatchCorrespondenceGen::deleteUnreliablePatches(double minFractionVisible, int minTimesVisibleForSensibleMeasurement)
{
    int deletedPatches = 0;
    for (int i=0; i<m_nPatches; i++)
    {
   if ( m_texturePatches[i]->getTimesSeen() >= minTimesVisibleForSensibleMeasurement)
   {
       double fractionOfTimesWasInlier = (double)(m_texturePatches[i]->getTimesInlier())/(double)(m_texturePatches[i]->getTimesSeen());
       if ( fractionOfTimesWasInlier < minFractionVisible)
       {
      //double imx, imy;
      //int id = m_PoseEst->getPointObservation(i, &imx, &imy);
      //printf("Deleting patch with index=%d and unique ID=%d (last seen at %6.2f %6.2f) as it is was visible only %6.2f of the time and was seen %d times\n", i, id, imx, imy, fractionOfTimesWasInlier, m_texturePatches[i]->getTimesSeen());
      this->deletePatch(i);
      deletedPatches++;
      i--;  // so we go back and look at the one we've just moved into this slot
       }

   }
    }
    return (deletedPatches);
}


bool PatchCorrespondenceGen::deletePatch(int i)
{
    // delete this patch by swapping pointers between this one and the end one, then reducing the
    // number of features by one, and making corresponding chages to the world point arrays
    if (i >= 0 && i <= m_nPatches)
    {
   TexturePatch *patchToDelete =  m_texturePatches[i];
   m_texturePatches[i] = m_texturePatches[m_nPatches-1];
   m_texturePatches[m_nPatches-1] = patchToDelete;
   m_nPatches--;
   return (m_PoseEst->deleteObject(i, m_nPatches));  // move object data from what was m_nPatches-1 before "m_nPatches--"
    }
    else
    {
   printf("PatchCorrespondenceGen::deletePatch: asked to delete patch %d which is outside range 0..%d\n",
          i, m_nPatches-1);
   return (false);
    }
}

void PatchCorrespondenceGen::drawProjectedPatches(unsigned char *image, int incX, int incY, int lenX, int lenY)
{
    //printf("PatchCorrespondenceGen::drawPatches: drawing %d patches...\n",m_nPatches);
    for (int i=0; i<m_nPatches; i++)
    {
   double imX, imY;
   if (m_PoseEst->projectIntoImageObjectPoint(0, &imX, &imY, true, i))    // the "true" means compensate for distortion
       // (could write a method of PoseEstPanTiltHead that gets the pose from the index)
   {
       //printf("  Patch %d projects to %6.2f,%6.2f\n", i, imX, imY);
       FittedLine hLine(imX-((float)m_patchLenX-1)/2.0, imY, imX+((float)m_patchLenX-1)/2.0, imY);
       FittedLine vLine(imX, imY-((float)m_patchLenY-1)/2.0, imX, imY+((float)m_patchLenY-1)/2.0);
       hLine.drawLine(image, incX, incY, lenX, lenY);
       vLine.drawLine(image, incX, incY, lenX, lenY);
   } // if visible
   //else printf("Patch %d falls outside image\n", i);
    } // for i=number of patches
}

void PatchCorrespondenceGen::drawPermanentPatches(unsigned char *image, int incX, int incY, int lenX, int lenY)
{
    for (int i=0; i<m_nPatches; i++)
    {
   double imX, imY;
   if (m_texturePatches[i]->getExpiryTime() < 0)
   {
       if (m_PoseEst->projectIntoImageObjectPoint(0, &imX, &imY, true, i))    // the "true" means compensate for distortion
       {
      FittedLine hLine0(imX-((float)m_patchLenX-1)/2.0, imY-((float)m_patchLenY-1)/2.0,
              imX+((float)m_patchLenX-1)/2.0, imY-((float)m_patchLenY-1)/2.0);
      FittedLine hLine1(imX-((float)m_patchLenX-1)/2.0, imY+((float)m_patchLenY-1)/2.0,
              imX+((float)m_patchLenX-1)/2.0, imY+((float)m_patchLenY-1)/2.0);
      FittedLine vLine0(imX-((float)m_patchLenX-1)/2.0, imY-((float)m_patchLenY-1)/2.0,
              imX-((float)m_patchLenX-1)/2.0, imY+((float)m_patchLenY-1)/2.0);
      FittedLine vLine1(imX+((float)m_patchLenX-1)/2.0, imY-((float)m_patchLenY-1)/2.0,
              imX+((float)m_patchLenX-1)/2.0, imY+((float)m_patchLenY-1)/2.0);
      hLine0.drawLine(image, incX, incY, lenX, lenY);
      hLine1.drawLine(image, incX, incY, lenX, lenY);
      vLine0.drawLine(image, incX, incY, lenX, lenY);
      vLine1.drawLine(image, incX, incY, lenX, lenY);
       } // if visible
   }  // if negative expiry time
    } // for i=number of patches
}

void PatchCorrespondenceGen::drawPermanentPatchesClassification(unsigned char *image,unsigned char *imageAlt, int incX, int incY, int lenX, int lenY)
{
    for (int i=0; i<m_nPatches; i++)
    {
   double imX, imY;
   if (m_texturePatches[i]->getExpiryTime() == -1)
   {
       if (m_PoseEst->projectIntoImageObjectPoint(0, &imX, &imY, true, i))    // the "true" means compensate for distortion
       {
      FittedLine hLine0(imX-((float)m_patchLenX-1)/2.0, imY-((float)m_patchLenY-1)/2.0,
              imX+((float)m_patchLenX-1)/2.0, imY-((float)m_patchLenY-1)/2.0);
      FittedLine hLine1(imX-((float)m_patchLenX-1)/2.0, imY+((float)m_patchLenY-1)/2.0,
              imX+((float)m_patchLenX-1)/2.0, imY+((float)m_patchLenY-1)/2.0);
      FittedLine vLine0(imX-((float)m_patchLenX-1)/2.0, imY-((float)m_patchLenY-1)/2.0,
              imX-((float)m_patchLenX-1)/2.0, imY+((float)m_patchLenY-1)/2.0);
      FittedLine vLine1(imX+((float)m_patchLenX-1)/2.0, imY-((float)m_patchLenY-1)/2.0,
              imX+((float)m_patchLenX-1)/2.0, imY+((float)m_patchLenY-1)/2.0);
      hLine0.drawLine(image, incX, incY, lenX, lenY);
      hLine1.drawLine(image, incX, incY, lenX, lenY);
      vLine0.drawLine(image, incX, incY, lenX, lenY);
      vLine1.drawLine(image, incX, incY, lenX, lenY);
       } // if visible
   }  // if negative expiry time
        else if (m_texturePatches[i]->getExpiryTime() < -1)
   {
       if (m_PoseEst->projectIntoImageObjectPoint(0, &imX, &imY, true, i))    // the "true" means compensate for distortion
       {
      FittedLine hLine0(imX-((float)m_patchLenX-1)/2.0, imY-((float)m_patchLenY-1)/2.0,
              imX+((float)m_patchLenX-1)/2.0, imY-((float)m_patchLenY-1)/2.0);
      FittedLine hLine1(imX-((float)m_patchLenX-1)/2.0, imY+((float)m_patchLenY-1)/2.0,
              imX+((float)m_patchLenX-1)/2.0, imY+((float)m_patchLenY-1)/2.0);
      FittedLine vLine0(imX-((float)m_patchLenX-1)/2.0, imY-((float)m_patchLenY-1)/2.0,
              imX-((float)m_patchLenX-1)/2.0, imY+((float)m_patchLenY-1)/2.0);
      FittedLine vLine1(imX+((float)m_patchLenX-1)/2.0, imY-((float)m_patchLenY-1)/2.0,
              imX+((float)m_patchLenX-1)/2.0, imY+((float)m_patchLenY-1)/2.0);
      hLine0.drawLine(imageAlt, incX, incY, lenX, lenY);
      hLine1.drawLine(imageAlt, incX, incY, lenX, lenY);
      vLine0.drawLine(imageAlt, incX, incY, lenX, lenY);
      vLine1.drawLine(imageAlt, incX, incY, lenX, lenY);
//       hLine0.drawLine(image, incX, incY, lenX, lenY);
//       hLine1.drawLine(image, incX, incY, lenX, lenY);
//       vLine0.drawLine(image, incX, incY, lenX, lenY);
//       vLine1.drawLine(image, incX, incY, lenX, lenY);
       } // if visible
   }  // if negative expiry time

    } // for i=number of patches
}


int PatchCorrespondenceGen::findCorrespondences(double matchErrToHalveWeight,
						bool usePrevIm,
						bool useStoredPatches,
						bool stopIfWeightDrops,
						bool normaliseDC,
						int highestLevelStoredPatches,
						int lowestLevelStoredPatches,
						int lowestLevelPrevPatches,
                                                double maxDiscardFraction,
                                                double discardRadius,
						double minScaleChangeToComputeScaledPatches,
						double maxWeight,
						bool   useWeights,
						double weightScale)
{
    bool projectWithDistortion=true;  // whether to add distortion when projecting points, when working out where to look
    // if (projectWithDistortion) printf("PatchCorrespondenceGen::findCorrespondences: projecting with distortion\n");
    if (highestLevelStoredPatches < 0) highestLevelStoredPatches = m_nResolutions-1;  // -1 means start from highest level
    // Clear point observations stored in PoseEstPanTiltHead object prior to adding the currently-visible ones
    m_PoseEst->clearAllPoints();

    // clear the counts of the number of points seen in each "region"
    for (int x=0; x<m_nRegionsX; x++) for (int y=0; y<m_nRegionsY; y++) m_nPatchesSeen[x][y] = 0;

    // note the current image velocity (in pixels/lines) and the exsimated focal length
    double curVelX, curVelY, estFocLen;
    m_PoseEst->getCurrentImageVelocity(&curVelX, &curVelY);
    m_PoseEst->getCamFocalLength(&estFocLen);

    // int notedForLearning=0, notNotedForLearning=0, nVisible=0;  // for testing!
    // first, project each patch into the camera using the PoseEst object - this gives the
    // location of each patch in the previous image.  Store the patch positions in an array.
    //  Also initialise a "status" array for each patch to indicate whether it has got "lost".
    //printf(" Checking visibility for %d patches...\n", m_nPatches);//  --21.6.09: visibility now set later
    int nPatchesVisible=0;  // for checking we don't throw out too many using motion-based ignore region
    int nPatchesDiscardedByMotionRegion=0;
    for (int i=0; i<m_nPatches; i++)
    {
      //printf("PatchCorrespondenceGen::findCorrespondences: trying patch %d.... \n", i);
   m_patchVisible[i] = false;   //assume patch not visible unless successfully tracked later
   if (m_PoseEst->projectIntoImageObjectPoint(0, &m_imX[i], &m_imY[i], projectWithDistortion, i))  // ith patch is point 0 on object i (optional  distortion)
   {
       nPatchesVisible++;
       //m_patchVisible[i] = true;  -- removed this 21.6.09: set it only when actually search for it later as may not have space in patchPos arrays to hold it
       //nVisible++;
       //printf("visible\n");
       // predicted point position (from pose of last image) lies within image...

       // note that a patch was seen in this part of the image
       // (note this even if we end up not using it: otherwise if this is due to a moving object obscuring a patch, we create a new patch
       //   on top of this object)
       int rx = (int)m_imX[i] / m_spacingX;
       int ry = (int)m_imY[i] / m_spacingY;

       // previously (v1.6 & earlier) we just counted the patches... now add code to remember the positions too so that
       // we can try not doing the fullClosenessCheck and still be able to check min dist
       //m_nPatchesSeen[rx][ry]++;
       if ( m_nPatchesSeen[rx][ry] < m_maxPatchesPerRegionForDistanceCheck)  // if space to store it.....
       {
      // retrieve the velocity when this patch was captured - we need this when deciding whether to note the patch position for
      // future reference when patch-finding, and could also check it against the current image velocity if we only wanted to use patches
      // captured close to the current motion speed (but this isn't implemented and may never be!).
      double patchVelX, patchVelY;
      m_texturePatches[i]->getVelocity(&patchVelX, &patchVelY);

      // retrieve the focal length when this patch was captured - we need this when deciding whether to note the patch position for
      // future reference when patch-finding ( S.W 12.10.09 )
      double patchFocLen = m_texturePatches[i]->getFocalLength();

      int p = m_nPatchesSeen[rx][ry];
      m_patchPosX[rx][ry][p] = int(m_imX[i]+0.5);
      m_patchPosY[rx][ry][p] = int(m_imY[i]+0.5);
      m_patchIndex[rx][ry][p] = i;  // index of the patch - needed for possible patch pruning later
      m_nPatchesSeen[rx][ry]=p+1;

      if (fabs(curVelX - patchVelX) < m_VelocityRangeForNoRelearningX &&     // if it is in the relevant velocity range to inhibit learning around here...
          fabs(curVelY - patchVelY) < m_VelocityRangeForNoRelearningY  )
      {
          //notedForLearning++;
          // printf("Patch %d which the %dth patch in block %d,%d with vel=(%6.2f,%6.2f) is close enough to current vel of (%6.2f,%6.2f)\n", i, p, rx, ry, patchVelX, patchVelY, curVelX,  curVelY);
          m_notePatchWhenLearning[rx][ry][p] = true;                  // then note it
      }
      else
      {
          //notNotedForLearning++;
          //printf("PatchCorrespondenceGen::findCorrespondences: not counting patch %d in region %d,%d when noting for later relearning\n", p, rx, ry);
          m_notePatchWhenLearning[rx][ry][p] = false;
      }

      if ( m_notePatchWhenLearning[rx][ry][p] )
      {
          //
          // Check the focal length when this patch was captured against the current focal length, to decide whether to take note of it when learning patches
          // The idea is to relearn patches when the focal length has changed sufficiently, in the same way as done when the velocity has changed.
          // S.W 12.10.09
          //
          double focLenRatioChange = patchFocLen / estFocLen;
          if (focLenRatioChange < 1.0) focLenRatioChange = 1.0 / focLenRatioChange;  // express as a ratio greater than 1.0
          if ( focLenRatioChange < m_FocLenRatioForNoRelearning) {
         m_notePatchWhenLearning[rx][ry][p] = true;
          } else {
         m_notePatchWhenLearning[rx][ry][p] = false;
          }
      }

       }  // if space to store the patch
       //else printf("Exceeded number of patches for distance check!!\n"); //prints too much debug


   }  // if point projects into image
   //else printf("  patch %i projects outside image\n", i);
    } // loop for ith patch
    //printf("nPatchesVisible = %d\n", nPatchesVisible);
    //printf("PatchCorrespondenceGen::findCorrespondences: out of %d patches, %d noted for learning and %d not noted for learning (total %d) out of %d visible\n",
    //   m_nPatches, notedForLearning, notNotedForLearning, notedForLearning+notNotedForLearning, nVisible);

// loop for each region, first checking whether we need to reduce the number of patches to search for
    for (int rx=0; rx<m_nRegionsX; rx++)
    {
   for (int ry=0; ry<m_nRegionsY; ry++)
   {
       if (m_nPatchesSeen[rx][ry] > m_maxPatchesToSearchPerRegion)
       {
      //printf("Exceeded max patches to search in region %d,%d\n", rx, ry);
      // for speed & neatness, note pointers to the arrays we'll use
      int *patchPosX=m_patchPosX[rx][ry];
      int *patchPosY=m_patchPosX[rx][ry];
      int *patchIndex = m_patchIndex[rx][ry];
      bool *notePatchWhenLearning = m_notePatchWhenLearning[rx][ry];

      // sort the patches so that the ones most likely to be useful are first, so when we limit the number we try,
      // we'll end up using the most appropriate ones
      for (int p=0; p<m_nPatchesSeen[rx][ry]; p++)
      {
          int i=patchIndex[p];
          // sort patches based on how useful they are likely to be for the current image:
          // - decrease the usefulness by a given fraction of the x & y speed differences
          // - also decrease it by a given fraction of the ratio of focal lengths between capture and now
          m_usefulness[p] = 1.0;  // arbitrary start value - could even use zero

          double patchVelX, patchVelY;
          m_texturePatches[i]->getVelocity(&patchVelX, &patchVelY);
          m_usefulness[p] -= fabs(curVelX - patchVelX) * m_patchPrefVelWeight;
          m_usefulness[p] -= fabs(curVelY - patchVelY) * m_patchPrefVelWeight;

          double focLenRatioChange = m_texturePatches[i]->getFocalLength() / estFocLen;
          if (focLenRatioChange < 1.0) focLenRatioChange = 1.0 / focLenRatioChange;  // express as a ratio greater than 1.0
          m_usefulness[p] -= (focLenRatioChange - 1.0) * m_patchPrefFoclenWeight;

          //printf("Patch index %d has usefulness %f\n", i, m_usefulness[p]);

      } // loop for each patch in region, measuring usefulness

      // sort m_patchPosX/Y, m_patchIndex, m_notePatchWhenLearning according to m_usefulnes
      for (int p=0; p<m_nPatchesSeen[rx][ry]-1; p++)        // standard bubble sort: compare this element....
      {
          for (int q=p+1; q<m_nPatchesSeen[rx][ry]; q++)    // with this following element
          {
         if (m_usefulness[q] > m_usefulness[p])
         {
             // swap all elements at p & q
             double tempd;
             tempd=m_usefulness[q]; m_usefulness[q]=m_usefulness[p]; m_usefulness[p]=tempd;
             int tempi=patchPosX[q]; patchPosX[q]=patchPosX[p]; patchPosX[p]=tempi;
             tempi=patchPosY[q]; patchPosY[q]=patchPosX[p]; patchPosY[p]=tempi;
             tempi = patchIndex[q]; patchIndex[q]=patchIndex[p]; patchIndex[p]=tempi;
             bool tempb = notePatchWhenLearning[q]; notePatchWhenLearning[q]=notePatchWhenLearning[p]; notePatchWhenLearning[p]=tempb;
         }
          }
      }  // loop for p=first patch to comapre in sort
      //printf("After sort:\n");
      //for (int p=0; p<m_nPatchesSeen[rx][ry]-1; p++)
      //  printf("Patch index %d has usefulness %f\n", patchIndex[p], m_usefulness[p]);

       }  // if saw more patches than we can handle and thus needed to sort

       for (int p=0; p<MIN(m_nPatchesSeen[rx][ry], m_maxPatchesToSearchPerRegion); p++)
       {
      int i=m_patchIndex[rx][ry][p];
      int levelUsed = -1;
      m_kltResult[i] = 0.0;

      // check for patch being in motion mask area
      // (note that predicted patch positions are based on previous camera pose, and mask is based on match of current image to previous image)
      bool discard = false;
      // check all points, centred on patch position, going out to corners of patch at full resolution scale
      if (m_useMotionMaskAsIgnoreRegion && m_motionMaskResolution != -1  &&
          (double)nPatchesDiscardedByMotionRegion / (double)nPatchesVisible < m_maxFractionToDiscardUsingMotionMask)
        {
          int fx=(int)m_imX[i]; int fy=(int)m_imY[i];
          //printf("findCorrespondences: Testing patch %d that projects to %d,%d against motion mask at resolution %d...\n", i, fx, fy, m_motionMaskResolution);
          int masklenX = m_lenX >> m_motionMaskResolution;
          int masklenY = m_lenY >> m_motionMaskResolution;
          int dy = ((m_patchLenY-1)/2)>>m_motionMaskResolution; if (dy<1) dy=1;
          int starty = (fy>>m_motionMaskResolution) - dy;
          int endy   = (fy>>m_motionMaskResolution) + dy;
          int dx = ((m_patchLenX-1)/2)>>m_motionMaskResolution; if (dx<1) dx=1;
          int startx = (fx>>m_motionMaskResolution) - dx;
          int endx   = (fx>>m_motionMaskResolution) + dx;
          //printf("startx=%d endx=%d dx=%d\n", startx, endx, dx);
          //printf("starty=%d endy=%d dy=%d\n", starty, endy, dy);
          for (int yy =  starty; yy <= endy && !discard; yy++)
            {
         if (yy >= 0 && yy < masklenY)
           {
             bool *motionMask = m_motionMask + yy*masklenX;
             for (int xx =  startx; xx <= endx && !discard; xx++)
               {
            if (xx >= 0 && xx < masklenX)
              {
                //printf("  Testing point %d,%d in motion mask: value=%d\n", xx, yy, (int)motionMask[xx]);
                discard = discard || motionMask[xx];  // discard if any tested pixel falls in mask area
                //if (motionMask[xx]) printf("findCorrespondences: Ignored a feature at %d %d due to it falling in motion mask\n", fx, fy);
              }
               }
           }
            }
          if (discard) nPatchesDiscardedByMotionRegion++;  // note that one more patch has been discarded
          //if (discard) printf("   patch discarded\n"); else printf("   patch not discarded\n");
        }
      // else printf("PatchCorrespondenceGen::findCorrespondences: Skipped check for motion mask as already discarded more than half of visible patches\n");

      if (!discard)
        {
          // (optionally) use the previous image pyramid to locate the new point positions in the current
          // image (but NB the previous image MUST correspond to the pose currently in PoseEst).  This will
          // update the positions in the array so that they correspond to the positions in the current image.
          if (usePrevIm && m_nImagesLoaded > 1)  // can only use prev image if we've loaded at least two images
            {
         // compute where we'll expect to find the patch at each level in the previous image
         for (int l=lowestLevelPrevPatches; l<m_nResolutions; l++)
           {
             m_prevPosX[l] = m_imX[i] / (double)(1<<l);
             m_prevPosY[l] = m_imY[i] / (double)(1<<l);
           }

         //printf("  Patch %d: initial pos est= %6.2f,%6.2f ",  i, m_imX[i], m_imY[i]);
         m_kltResult[i] = m_KLTTracker->multiLevelTrack(
                               m_image[m_cur], m_image[m_prev],  // pointers to arrays of pointers to top-left pixel of each level
                               m_lenX, m_lenY,               // dims of image at top level (other level dims are derived from these)
                               m_lenX, m_lenY,               // dims of image at top level (other level dims are derived from these)
                               m_incX, m_incY,               // arrays giving x & y increments for each level (current image)
                               m_incX, m_incY,               // arrays giving x & y increments for each level (previous image)
                               m_nResolutions-1,             // highest level to use
                               lowestLevelPrevPatches,       // lowest level to use
                               &m_imX[i], &m_imY[i],             // initial estimate (in) and final result (out) for position in current image
                               m_prevPosX, m_prevPosY,       // position of point in each level of previous image to match (w.r.t top=left pixel in prev image)
                               //        m_imX[i], m_imY[i],               // position of point in previous image to match (w.r.t top=left pixel in prev image)
                               m_patchLenX, m_patchLenY,     // dims of patch to use (centred on given point)
                               false,                        // no DC normalisation - may not be needed when tracking from prev im
                               false,                        // all levels in prev im are NOT the same size (not a ref block)
                               stopIfWeightDrops,            // whether to stop if weight drops rather than increases as level drops
                               &levelUsed,                   // return the level that was used here
                               m_directionalWeight[i]);        // put the directional weighting (eigenvectors) in here
         //printf("  multiLevelTrack returned %6.2f and gave location as %6.2f,%6.2f\n", m_kltResult[i], m_imX[i], m_imY[i]);
         //printf("  Using prev patches: patch %02d: multiLevelTrack used level %d\n", i, levelUsed);
         m_patchVisible[i] = (m_kltResult[i] >=0.0);  // note if patch was tracked OK

            }  // if tracking from prev image

          // then (optionally) use the stored images for each patch to refine the patch positions - using
          // DC normalisation.  This should prevent drift.
          if (m_kltResult[i] >= 0.0 && useStoredPatches)
            {
         // if the ratio of the scale change from when the patch was captured to now is above the
         // minimum we care about, then warp the stored patch so that the patch & associated data that
         // we use below has been compensated for the change in focal length
         if ( fabs( (m_texturePatches[i]->getFocalLength() / estFocLen) - 1.0) > minScaleChangeToComputeScaledPatches)
           {
	     //int id; double xx, yy; id=m_PoseEst->getPointObservation(i, &xx, &yy);
             //printf("Warping patch %d (ID=%d)to change focal length from %7.5f to %7.5f - ratio of %6.3f...\n", i, id,
             //   m_texturePatches[i]->getFocalLength(), estFocLen, estFocLen/m_texturePatches[i]->getFocalLength());
             m_texturePatches[i]->generateWarpedPatch(estFocLen, lowestLevelStoredPatches, highestLevelStoredPatches);
             m_texturePatches[i]->useWarpedPatch(true);
           }
         else
           m_texturePatches[i]->useWarpedPatch(false);

         m_kltResult[i] = m_KLTTracker->multiLevelTrack(
                               m_image[m_cur], m_texturePatches[i]->getImagePointerArray(),  // pointers to arrays of pointers to top-left pixel of each level
                               m_lenX, m_lenY,               // dims of image at top level (other level dims are derived from these)
                               m_patchLenX, m_patchLenY,     // dims of prev image - this is the patch size
                               m_incX, m_incY,               // arrays giving x & y increments for each level (current im)
                               m_texturePatches[i]->getIncXArray(), m_texturePatches[i]->getIncYArray(), // incs in levels of stored patches
                               highestLevelStoredPatches,             // highest level to use
                               lowestLevelStoredPatches,              // lowest level to use
                               &m_imX[i], &m_imY[i],             // initial estimate (in) and final result (out) for position in current image
                               m_texturePatches[i]->getFeaturePosXArray(), m_texturePatches[i]->getFeaturePosYArray(),  // pos of feature w.r.t. top-left of patch at each level
                               //        int((m_patchLenX-1)/2), int((m_patchLenY-1)/2),   // position of point in centre of patch to match, rounded down as it is during patch creation
                               m_patchLenX, m_patchLenY,     // dims of patch to use (centred on given point)
                               normaliseDC,                  // whether to normalise DC level of patches
                               true,                         // true means prev levels are all the same size
                               stopIfWeightDrops,            // whether to stop if weight drops rather than increases as level drops
                               &levelUsed,                   // return the level that was used here
                               m_directionalWeight[i],         // put the directional weighting (eigenvectors) in here
                               m_texturePatches[i]->getMissingX0Array(),
                               m_texturePatches[i]->getMissingX1Array(),
                               m_texturePatches[i]->getMissingY0Array(),
                               m_texturePatches[i]->getMissingY1Array());

         m_patchVisible[i] = (m_kltResult[i] >=0.0);  // note if patch was tracked OK

         //printf("  Using stored patches: multiLevelTrack returned %6.2f and gave location as %6.2f,%6.2f\n", m_kltResult[i], m_imX[i], m_imY[i]);
         //printf("  Using stored patches: patch %02d: multiLevelTrack used level %d\n", i, levelUsed);
            }  // if refining with stored patch and got good enough match before

        } // if not discarded by motion mask

       }  // loop for p=each patch in region rx,ry
    }  // loop for ry
}  // loop for rx

    //
    // Count correspondences found
    //
    int correspondenciesFound = 0;

    for (int i=0; i<m_nPatches; i++)
      {
	if ( m_patchVisible[i] )
	  {
	    if (m_kltResult[i] >= 0.0 )
	      {
		correspondenciesFound++;
	      }
	  }
	
      }
    
    //
    // Add correspondences to PoseEstimator
    //
    int correspondenciesUsed = 0;
    int correspondenciesDiscarded = 0;
    double cx = (m_lenX/2.0);
    double cy = (m_lenY/2.0);
    double discardRadiusSquared = discardRadius * discardRadius;
    for (int i=0; i<m_nPatches; i++)
    {
   if ( m_patchVisible[i] )
        {
       //printf("PatchCorrGen::findCorr: patch %3d: m_kltResult=%6.2f pos=(%5.1f,%5.1f) weights=%5.1f %5.1f %5.1f %5.1f", i, m_kltResult[i],
       //   m_imX[i], m_imY[i], m_directionalWeight[i][0], m_directionalWeight[i][1], m_directionalWeight[i][2], m_directionalWeight[i][3]);
       if (m_kltResult[i] >= 0.0 )
            {
      double xCentred = (m_imX[i]-cx);
      //double yCentred = (m_imY[i]-cy)*2.0; // temporary fix for fields S.W. 22/10/07
      double yCentred = (m_imY[i]-cy) * m_PoseEst->getPixelAspect(); // should give proper distance in unite of pixel width - use once checked operation with temp fix
      double screenRadiusSquared = xCentred*xCentred + yCentred*yCentred;
      double discardFraction = (double)correspondenciesDiscarded/(double)correspondenciesFound;

      if ( ( screenRadiusSquared < discardRadiusSquared ) && ( discardFraction < maxDiscardFraction ) )
      {
          correspondenciesDiscarded++;
      }
      else
      {
          if ( useWeights )
          {
         // compute weight based on match error: small errors don't affect weight; for large errors, doubling the error halves the weight:
         double weightFromMatchErr = (double)matchErrToHalveWeight / ((double)matchErrToHalveWeight + m_kltResult[i]);
         for (int w=0; w<4; w++) m_directionalWeight[i][w] = m_directionalWeight[i][w] * weightFromMatchErr * weightScale;

         if ( maxWeight > 0.0 ) {
             //
             // Clip weights above maxWeight.
             // The lengths of both eigenvectors are scaled down so that the longest has length maxWeight
             // and the shorter is scaled in proportion, so that the directions remain unchanged
             //
             double dw0 = m_directionalWeight[i][0];
             double dw1 = m_directionalWeight[i][1];
             double dw2 = m_directionalWeight[i][2];
             double dw3 = m_directionalWeight[i][3];
             double l0 = sqrt( dw0*dw0 + dw1*dw1 );
             double l1 = sqrt( dw2*dw2 + dw3*dw3 );
             double largestWeight = max( l0, l1 );
             if ( largestWeight > maxWeight ) {
            double clippingScale = maxWeight/largestWeight;
            for (int w=0; w<4; w++) m_directionalWeight[i][w] = m_directionalWeight[i][w] * clippingScale;
             }
         }
                    }  // if using weights
          else  // if not using weights....
          {
                        //
                        // Ignore the calculated weights
                        //
/*
//
// Set the lengths of both eigenvectors to 10, while maintaining their directions.
// This approach does not seem to work as well in practice as the simpler one below.
//
double dw0 = m_directionalWeight[i][0];
double dw1 = m_directionalWeight[i][1];
double dw2 = m_directionalWeight[i][2];
double dw3 = m_directionalWeight[i][3];
double l0 = sqrt( dw0*dw0 + dw1*dw1 );
double l1 = sqrt( dw2*dw2 + dw3*dw3 );
m_directionalWeight[i][0] = m_directionalWeight[i][0]/l0 * 10.0;
m_directionalWeight[i][1] = m_directionalWeight[i][1]/l0 * 10.0;
m_directionalWeight[i][2] = m_directionalWeight[i][2]/l1 * 10.0;
m_directionalWeight[i][3] = m_directionalWeight[i][3]/l1 * 10.0;
*/
                        //
                        // Set the eigenvectors to constant values, ignoring both weight and directional information.
                        //
         m_directionalWeight[i][0] = weightScale;  // NB was previously set to 10.0
         m_directionalWeight[i][1] = 0.0;
         m_directionalWeight[i][2] = 0.0;
         m_directionalWeight[i][3] = weightScale;
                    }   // if using weights or not

          // add this correspondence to the PoseEstPanTiltHead object
          m_PoseEst->addPointObservation(i, m_imX[i], m_imY[i],
                     1.0,  // weight defaults to 1.0 for now - could mkae it dependant on match err, for example
                     m_directionalWeight[i]);
          correspondenciesUsed++;
          //printf(" -> patch used");
      }
            }  // if good enough match to use
       //printf("\n");
        }
    }

//  double discardFraction = (double)correspondenciesDiscarded/(double)correspondenciesFound;
//  printf( "< PatchCorrespondenceGen::findCorrespondencies discarded %d out of %d correspondences %6.2f\n",
//    correspondenciesDiscarded, correspondenciesFound, discardFraction
//  );
    return (correspondenciesUsed);
}

int PatchCorrespondenceGen::calcCorrespondenciesGlobally(
    bool useStoredPatches,  // true=from stored patches, false=from previous image
    bool normaliseDC,       // whether to normalise DC
    int highestLevel,       // most subsampled level to use  (-1=use hightest available)
    int lowestLevel,        // least subsampled level to use (0=full-res)
    double stopIterShift, double stopIterScaleChange, int stopIterCount)  // controls for iteration stopping
{

    //  Note: think about whether there is a better way of identifying outliers than a full KLT/RANSAC approach
    //    Could also consider reverting the pose computed after KLT/RANSAC back to its previously-estimated value, and letting
    //    this method compute it from the original starting point: this should stop there from being any roll (?)

    // TODO: add option to compute rescaled patches; this would be useful if this global method was the only one used

    if (highestLevel < 0) highestLevel = m_nResolutions-1;  // -1 means start from highest level
    if (highestLevel < lowestLevel) { printf("PatchCorrespondenceGen::calcCorrespondenciesGlobally: highestLevel < lowestLevel!!\n"); exit (-1); }

    // first, project each patch into the camera using the PoseEst object - this gives the
    // location of each patch in the previous image.  Store the patch positions in an array and note if point not visible
    for (int i=0; i<m_nPatches; i++)
    {
   m_patchVisible[i] = m_PoseEst->projectIntoImageObjectPoint(0, &m_imX[i], &m_imY[i], false, i);
    }

    if (0)  // for testing global computation by adding offsets to initial estimates of block positions
    {
   double dx=0.1, dy=0.2, ds=0.003;
   printf("CalcCorrespGlobally: deliberately introducing a shift of %f,%f and scale chage of %f to predicted locations\n", dx, dy, ds);
   for (int i=0; i<m_nPatches; i++)
   {
       // compute predicted feature loc using curently-estimated shift and scale change
       m_imX[i] += dx + ds*m_imX[i];
       m_imY[i] += dy + ds*m_imY[i];
   }

    }

    Solve mySolver;  // for solving the linear equations computed
    int retCode = 0;

    // now we know the predicted location for each patch, we can build a set of equations using contributions from pixels in all patches,
    // to estimate the global tranlation and scale change of the current image compared to the reference patches

    int nUnknowns = 3;        // solve for x, y shift and scale change for now (could expand to roll later)
    double eqns[12];          // storage for (nUnknowns x nUnknowns) coeffs, plus nUnknowns right-hand-sides for computing shift and scale change
    double unknowns[3];       // computed xshift, yshift, (scaleChange-1)

    double xShift = 0.0, yShift=0.0, scaleChange=0.0;  // currently-estimated changes w.r.t. values computed by projectIntoImageArbitraryPoint

    for (int l=highestLevel; l>=lowestLevel; l--)  // loop over all requested levels of image pyramid
    {
   int scale = 1 << l;  // scale factor of this level

   bool iterating = true;
   int iterCount=0;

   while (iterating)    // iterate within the computation at this level to converge to a minimum
   {
       //printf("PatchCorrespondenceGen::calcCorrespondenciesGlobally: level %d (scale %d): starting iteration %d with %d stored patches....\n", l, scale, iterCount, m_nPatches);
       //printf("Current shift est = %6.2f,%6.2f,scale est=%f\n", xShift, yShift, scaleChange);
       for (int i=0; i<nUnknowns*(nUnknowns+1); i++) eqns[i] = 0.0;  // clear prior to accumulating contributions
       int patchesUsed=0;  // for testing

       for (int i=0; i<m_nPatches; i++)
       {
      // compute predicted feature loc using curently-estimated shift and scale change
      double predX = m_imX[i] * (1.0 + scaleChange) + xShift;
      double predY = m_imY[i] * (1.0 + scaleChange) + yShift;

      if (m_patchVisible[i] && m_PoseEst->pointWasAnInlier(i))  // only use visible points that were inliers when last tried
      {
          patchesUsed++;
          double absDiff;
          if (!useStoredPatches) // if using previous image.....
          {
         absDiff = m_KLTTracker->accumulateEqnContributions(
             m_image[m_cur][l], m_image[m_prev][l],  // pointers to arrays of pointers to top-left pixel of each level
             m_lenX / scale, m_lenY / scale,               // dims of image at top level for current
             m_lenX / scale, m_lenY / scale,               // dims of image at top level for previous
             m_incX[l], m_incY[l],                   // x & y increments for current image
             m_incX[l], m_incY[l],                   // x & y increments for prev image
             predX/(double)scale, predY/(double)scale,    // initial estimate for position in current image
             predX/(double)scale, predY/(double)scale,     // position of point in previous image to match (w.r.t top=left pixel in prev image)
             m_patchLenX, m_patchLenY,     // dims of patch to use (centred on given point)
             m_PoseEst->getPixelAspect(),   // pixel aspect ratio (width / length) - needed when computing roll
             nUnknowns, eqns,              // number of unknowns and where to put results
             false);                       // no DC normalisation - should not be needed when tracking from prev im
          }  // if tracking from prev image
          else  // ...we're maching stored patches to current image...
          {
         absDiff = m_KLTTracker->accumulateEqnContributions(
             m_image[m_cur][l], m_texturePatches[i]->getImagePointerArray()[l],  // pointers to arrays of pointers to top-left pixel of each level
             m_lenX / scale, m_lenY / scale,               // dims of current image at this level
             m_patchLenX, m_patchLenY,                    // dims of prev image - this is the patch size
             m_incX[l], m_incY[l],                   // x & y increments for current image
             m_texturePatches[i]->getIncXArray()[l], m_texturePatches[i]->getIncYArray()[l],   // x & y increments for prev image
             predX/(double)scale, predY/(double)scale,    // initial estimate for position in current image
             m_texturePatches[i]->getFeaturePosXArray()[l], m_texturePatches[i]->getFeaturePosYArray()[l],  // pos of feature w.r.t. top-left of patch at each level
//           int((m_patchLenX-1)/2), int((m_patchLenY-1)/2),   // position of point in centre of patch to match, rounded down as it is during patch creation
             m_patchLenX, m_patchLenY,     // dims of patch to use (centred on given point)
             m_PoseEst->getPixelAspect(),   // pixel aspect ratio (width / length)
             nUnknowns, eqns,              // number of unknowns and where to put results
             normaliseDC,                  // optional DC normalisation
             m_texturePatches[i]->getMissingX0Array()[l],
             m_texturePatches[i]->getMissingX1Array()[l],
             m_texturePatches[i]->getMissingY0Array()[l],
             m_texturePatches[i]->getMissingY1Array()[l]);
          }
          //printf("PatchCorrespondenceGen::calcCorrespondenciesGlobally: patch %d, orig pos %6.2f,%6.2f  scale=%d, pred pos %6.2f,%6.2f  abs diff=%f\n", i, m_imX[i], m_imY[i], scale, predX, predY, absDiff);
      }  // if patch visible
      //else
      //{
      //    if (m_PoseEst->pointWasAnInlier(i)) printf("PatchCorrespondenceGen::findCorrespondences: patch %3d: patch not visible\n", i);
      //    else printf("PatchCorrespondenceGen::findCorrespondences: patch %3d: patch was an outlier\n", i);
      //}
       }  // loop for i=each patch

       // compute shift and scale change from the accumulated equations
       int solverRetCode;
       if ( (solverRetCode = mySolver.solveTMatrix(eqns, unknowns, nUnknowns)) == Solve::SOLVE_SUCCESS)
       {
      // unpack computed shift and scale change, and put in units of full-res image
      double dx = eqns[nUnknowns*nUnknowns + 0] * (double) scale;
      double dy = eqns[nUnknowns*nUnknowns + 1] * (double) scale;
      double ds = eqns[nUnknowns*nUnknowns + 2];         // (scale change is independent of subsampling applied to image!)

      // add these shifts and scale change to what we had before
      xShift      += dx;
      yShift      += dy;
      scaleChange  += ds;

      //printf("  solveTMatrix returned %d, and gave a shift of %f,%f and scalechange of %f\n", solverRetCode, dx, dy, scaleChange);

      // work out if we should stop iterating at this level
      iterCount++;
      if ( ((dx*dx+dy*dy < stopIterShift*stopIterShift) && (fabs(ds) < stopIterScaleChange)) || iterCount >= stopIterCount)
      {
          iterating = false;
          retCode = 0;
      }
       }
       else
       {
      // printf("PatchCorrespondenceGen::calcCorrespondenciesGlobally: solveTMatrix returned a failure code of %d\n",retCode);
      iterating=false;  // stop iterating if equations unsolvable
      retCode = -1;     // note error condition for returning
       }

   }  // while iterating at this level

    }    // for l=level

    // update observed feature locations with newly-refined values
    // Clear point observations stored in PoseEstPanTiltHead object prior to adding the currently-visible refined ones
    // NB the inlier array in PoseEStPanTiltHead is unaffected by this clear operation, and we compute 'ideal' feature locations for
    // both inliers and outliers, even though the observed 2D locations of outliers aren't drawn
    m_PoseEst->clearAllPoints();

    if (retCode == 0)  // only if we never failed to converge...
    {
   double dummyDirWeight[4];
   dummyDirWeight[0] = 1.0;  // hard-code directional weight to 1.0
   dummyDirWeight[1] = 0.0;
   dummyDirWeight[2] = 0.0;
   dummyDirWeight[3] = 1.0;
   for (int i=0; i<m_nPatches; i++)
   {
       if ( m_patchVisible[i] )
       {
      double predX = m_imX[i] * (1.0 + scaleChange) + xShift;
      double predY = m_imY[i] * (1.0 + scaleChange) + yShift;
      m_PoseEst->addPointObservation(i, predX, predY, 1.0, dummyDirWeight);
      retCode++;  // count number of visible patches that we're updating
       }
   }
    }

    //test: count outliers
    int nOutliers = 0;
    int nVisible = 0;
    for (int i=0; i<m_nPatches; i++)
    {
   if ( m_patchVisible[i] )
   {
       nVisible++;
       if (!m_PoseEst->pointWasAnInlier(i)) nOutliers++;
   }
    }
    //printf("  Visible points=%d, of which outliers=%d  out of %d total patches\n", nVisible, nOutliers, m_nPatches);
    return (retCode);
}





int PatchCorrespondenceGen::partition(const int top, const int bottom)
{

    double x = match_array[top].prob;
    int i = top - 1;
    int j = bottom + 1;
    coord_prob temp;

    do
    {
   do
   {
       j--;
   }while (x >match_array[j].prob);

   do
   {
       i++;
   } while (x <match_array[i].prob);

   if (i < j)
   {
       temp = match_array[i];    // switch elements at positions i and j
       match_array[i] = match_array[j];
       match_array[j] = temp;
   }
    }while (i < j);
    return j;           // returns middle index
}

void PatchCorrespondenceGen::quicksort(int top, int bottom)
{
    int middle;
    if (top < bottom)
    {
   middle = partition(top, bottom);
   quicksort(top, middle);   // sort top partition
   quicksort(middle+1, bottom);    // sort bottom partition
    }
    return;
}




double PatchCorrespondenceGen::classifierInitialisation(double maxInlierDist, double minInlierFraction, int maxTries,
                     int nInitialPoints, int maxNewFeaturesToFind, int cycle, int subsample, double initialiseScale)
{

    if(!(rf))
    {
   printf("\nError. No classifier has been constructed\n");
   return 0.0;
    }

    timeval startTime, endTime;
    //gettimeofday(&startTime, 0);


    int nFound = this->findClassifierCorrespondences(0, maxNewFeaturesToFind, cycle, subsample, initialiseScale);

/*      gettimeofday(&endTime, 0);
   double initialiseTime = ( (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000000.0);
   printf("C: %f\n", initialiseTime);
   startTime = endTime;*/

    if ((nFound == -1)||(cycle == 0))
    {
   return 0.0;
    }


    if (debug)
    {
   printf("nFound: %d\n", match_array_index);
    }

// use RANSAC to try to get a solution, as long as we've got enough corresponencies

    double result = -1;

    if (match_array_index > nInitialPoints)  // would expect to have more than this generally
    {
   // pre-compute & store pitch rotation matrices, to speed this part up
   m_PoseEst->cachePitchRot(true);

   int nFinalPoints;

   result = m_PoseEst->calcLastSolutionRansac(maxInlierDist,
                     minInlierFraction,
                     maxTries,
                     nInitialPoints,
                     &nFinalPoints,
                     0, // nInitialLines
                     (int *)0, // *nFinalLines
                     1.0, // inlierDistLineScale
                     1,   //roughEstimateMode=1 for estimating pan/tilt from chosen points
                     false); // tryAllAsInliersFirst - set to false as most won't be inliers and roughEstimateMode=1 needs this false
//     printf("result: %f\n", result);
    }
/*      gettimeofday(&endTime, 0);
   initialiseTime = ( (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000000.0);
   printf("P: %f\n", initialiseTime);*/

    return (result);
}


double PatchCorrespondenceGen::exhaustiveSearch(double pMin, double pMax, double pStepPixels,
                  double tMin, double tMax, double tStepPixels,
                  double rMin, double rMax, int rNStep,
                  double fMin, double fMax, double fStepPixels,
                  double maxInlierDist, double minInlierFraction, int maxTries,
                  int nInitialPoints, int lenYFull)
{
    double pBest=0.0, tBest=0.0, rBest=0.0,  fBest=10.0;
    int nPointsBest = 0;
    double resultBest = -1.0;
    double retCode = -3.0;  // this return code would indicate we never found enough features to try RANSAC

    printf("PatchCorrespondenceGen::exhaustiveSearch: starting....\n");
    printf("pan  min=%6.2f  max=%6.2f pixelstep=%6.2f\n", pMin, pMax, pStepPixels);
    printf("tilt min=%6.2f  max=%6.2f pixelstep=%6.2f\n", tMin, tMax, tStepPixels);
    printf("fovy min=%6.2f  max=%6.2f pixelstep=%6.2f\n", fMin, fMax, fStepPixels);
    printf("roll min=%6.2f  max=%6.2f nstep=%d\n", rMin, rMax, rNStep);

    // pre-compute & store pitch rotation matrices, to speed this part up
    m_PoseEst->cachePitchRot(true);

    // loop for each possible set of values for pan, tilt, fov at current position
    double fovy = fMax;  // vert fov in degrees
    while (fovy >= fMin)
    {
   // work out horiz field-of-view (needed for computing pan step size)
   double fovx = 2 * atan( tan(fovy*RPD/2) * m_PoseEst->getAspect() ) / RPD;
   // if (m_verbose > 0)
   printf(" fovy = %6.2f   fovx=%6.2f\n", fovy, fovx);

   double panDir = 1.0;
   for (double tilt = tMin; tilt <= tMax; tilt += tStepPixels * fovy / (double)m_lenY)
   {
       //printf("  tilt = %6.2f\n", tilt);
       panDir = -panDir;  // scan pan direction alternately l-r and r-l in case we just miss some features coming in
       double panStep = pStepPixels  * fovx / (double)m_lenX * panDir;
       for (double pan = (panDir > 0 ? pMin :  pMax);
       (panDir > 0 ? pan <= pMax : pan >=pMin);
       pan  += panStep)
       {
      for (int r=0; r<rNStep; r++)
      {
          double roll;
          if (rNStep < 2) roll = rMin;
          else            roll =(double)r/(double)(rNStep-1)*(rMax-rMin) + rMin;
          printf("    trying pan = %6.2f   tilt=%f   fovy=%f  roll=%f\n", pan, tilt, fovy, roll);

          m_PoseEst->setCamRotPTR(pan, tilt, roll);
          m_PoseEst->setCamFOV(fovy, lenYFull);

          if (true)
          {

         // look for matches using just stored features, using full image pyramid, DC removal
         int nFound = this->findCorrespondences(999,   //para.matchThresh,
                            false, //para.usePrevIm,
                            true,  //para.useStoredPatches,
                            false, //para.stopIfWeightDrops,
                            true,  //para.normaliseDC,
                            -1);   //para.highestLevelStoredPatches

         // use RANSAC to try to get a solution, as long as we've got enough corresponencies
         if (nFound > nInitialPoints)  // would expect to have more than this generally
         {
             int nFinalPoints;
             double result = m_PoseEst->calcLastSolutionRansac(maxInlierDist,
                                 minInlierFraction,
                                 maxTries,
                                 nInitialPoints,
                                 &nFinalPoints);
             if (result >= 0.0 &&                                        // if we got a usable solution...
            (nFinalPoints > nPointsBest ||                          // and either used more points than before....
             (nFinalPoints == nPointsBest && result < resultBest))) // ... or used the same number and got a smaller error....
             {
            // the solution we just tried is the best so far
            pBest = pan;  tBest = tilt;  rBest = roll;  fBest=fovy;
            nPointsBest = nFinalPoints; resultBest = result;
             }
         }  // if enough points found to try RANSAC

          }  // if true/false
      } // r
       } // loop for pan

   }  // loop for tilt

   fovy -= fovy * fStepPixels / (double)m_lenY;
    }  // while fov > fMin

    m_PoseEst->setCamRotPTR(pBest, tBest, rBest);
    m_PoseEst->setCamFOV(fBest, lenYFull);

    // now we have a reasonable answer, do the feature search and RANSAC estimation again, as we may be able to get a more accurate
    // match and thus a better final result.
    // (Could actually do this for several of the best solutions.... could add this later)
    if (true)
    {
   int nFound = this->findCorrespondences(999,   //para.matchThresh,
                      false, //para.usePrevIm,
                      true,  //para.useStoredPatches,
                      false, //para.stopIfWeightDrops,
                      true,  //para.normaliseDC,
                      -1);   //para.highestLevelStoredPatches

   if (nFound > nInitialPoints)  // would expect to have more than this generally
   {
       resultBest=m_PoseEst->calcLastSolutionRansac(maxInlierDist,
                      minInlierFraction,
                      maxTries,
                      nInitialPoints,
                      &nPointsBest);
   }
    }
    return (resultBest);
}

bool PatchCorrespondenceGen::writePatches(char *fileName)
{
    FILE *fp;
#define PATCH_FILE_VERSION_STRING "Patch list v1.3\n"  // write at start of file & check

    if ( (fp = fopen(fileName, "w")) == NULL)
    {
   printf("Can't open patch file %s for writing\n",fileName);
   return(false);
    }
    else
    {
   fwrite((void *)PATCH_FILE_VERSION_STRING , 1, strlen(PATCH_FILE_VERSION_STRING), fp);   // put initial string at top of file
   fwrite((void *)&m_nPatches, sizeof(m_nPatches), 1, fp);    // number of precomputed locations as a 4-byte int
   fwrite((void *)&m_patchLenX, sizeof(m_patchLenX), 1, fp);    //  patch  dimensions
   fwrite((void *)&m_patchLenY, sizeof(m_patchLenY), 1, fp);    //  ditto
   fwrite((void *)&m_nResolutions, sizeof(m_nResolutions), 1, fp);    //  number  of resolution levels per patch
   double cx, cy, cz;
   m_PoseEst->getCamPos(&cx, &cy, &cz);            // position of camera
   fwrite((void *)&cx, sizeof(cx), 1, fp);
   fwrite((void *)&cy, sizeof(cy), 1, fp);
   fwrite((void *)&cz, sizeof(cz), 1, fp);
   double rx, ry, rz;
   m_PoseEst->getPitchRot(&rx, &ry, &rz);     // rotation of pitch
   fwrite((void *)&rx, sizeof(rx), 1, fp);
   fwrite((void *)&ry, sizeof(ry), 1, fp);
   fwrite((void *)&rz, sizeof(rz), 1, fp);

   //printf("PatchCorrespondenceGen::writePatches: writing cam pos as %6.2f,%6.2f,%6.2f\n", cx, cy, cz);
   bool wroteOK = true;
   for (int i=0; i<m_nPatches && wroteOK; i++)
   {
       // write the id and world coordinates of this patch
       double wx, wy, wz;
       int id = m_PoseEst->getPoint(i, &wx, &wy, &wz);   // (this is the pos in world w.r.t. the ref object)

       // we want to store the position of this point w.r.t. the origin of the reference object (which is the world origin by definition)
       // which is what the above method has given us

       //printf("Writing patch %d with world pos %6.2f,%6.2f,%6.2f\n", i, wx, wy, wz);
       // 12-6-09: write the ID - it is needed so reloaded patches tie up with the initialisation method
       // this means that we can't add loaded patches to ones already held, but we didn't do this anyway.
       fwrite((void *)&id, sizeof(id), 1, fp);
       fwrite((void *)&wx, sizeof(wx), 1, fp);
       fwrite((void *)&wy, sizeof(wy), 1, fp);
       fwrite((void *)&wz, sizeof(wz), 1, fp);

       wroteOK = m_texturePatches[i]->writePatch(fp);     // write all data that is specific to this patch
   }

   fclose (fp);
   return (wroteOK);

    } // if file opened OK
}

bool PatchCorrespondenceGen::readPatches(char *fileName)
{
    FILE *fp;

    if ( (fp = fopen(fileName, "r")) == NULL)
    {
   printf("Can't open patch file %s for reading\n",fileName);
   return(false);
    }
    else
    {
   // start by checking the file starts with the version string we expect.....
   bool readIDs=true;  // do this in v1.3 and above
   char *verString = new char[strlen(PATCH_FILE_VERSION_STRING)];
   fread((void *)verString, 1, strlen(PATCH_FILE_VERSION_STRING), fp);   // put initial string at top of file
   if (strncmp(verString, PATCH_FILE_VERSION_STRING, strlen(PATCH_FILE_VERSION_STRING)) != 0)
   {
       // allow a v1.2 file and not not to try rad patch IDs....
#define PATCH_FILE_VERSION_STRING_v1Point2 "Patch list v1.2\n"
       if (strncmp(verString, PATCH_FILE_VERSION_STRING_v1Point2, strlen(PATCH_FILE_VERSION_STRING)) == 0)
       {
      printf("Reading v1.2 patch file and generating sequential IDs...\n");
      readIDs=false;  //v.1 2 is like v1.3 but no IDs
       }
       else
       {
      printf("Error: patch file %s has the wrong format: %s\n",fileName, verString);
      return(false);
       }
   }
   delete verString;

   // for now, make these patches replace any that we already have, but could have an option to add them to those
   // already stored, after checking that patch dims and number of levels match those we already have.
   this->deleteAllPatches();

   int patchLenXFile, patchLenYFile, nResolutionsFile;
   fread((void *)&m_nPatches, sizeof(m_nPatches), 1, fp);    // number of precomputed locations as a 4-byte int
   fread((void *)&patchLenXFile, sizeof(patchLenXFile), 1, fp);    //  patch  dimensions
   fread((void *)&patchLenYFile, sizeof(patchLenYFile), 1, fp);    //  ditto
   fread((void *)&nResolutionsFile, sizeof(nResolutionsFile), 1, fp);    //  number  of resolution levels per patch
   double cx, cy, cz;
   fread((void *)&cx, sizeof(cx), 1, fp);
   fread((void *)&cy, sizeof(cy), 1, fp);
   fread((void *)&cz, sizeof(cz), 1, fp);
   m_PoseEst->setCamPos(cx, cy, cz);            // position of camera
   double rx, ry, rz;
   fread((void *)&rx, sizeof(rx), 1, fp);
   fread((void *)&ry, sizeof(ry), 1, fp);
   fread((void *)&rz, sizeof(rz), 1, fp);
   m_PoseEst->setPitchRot(rx, ry, rz);            // position of camera

   bool readOK = true;

   // make sure the patch dimensions and number of levels match what we've already alllcated space for
   // (for now, give up if they don't match but it would be possible to reallocate the space for different dims)
   if (patchLenXFile != m_patchLenX || patchLenYFile != m_patchLenY || nResolutionsFile != m_nResolutions)
   {
       printf("PatchCorrespondenceGen::readPatches: patch dims and/or number of levels do not match what I was expecting:\n");
       printf("File %s holds data for patches of dims %d x %d with %d resolution levels\n", fileName, patchLenXFile, patchLenYFile, nResolutionsFile);
       readOK=false;
   }

   if (m_nPatches > m_maxPatches-1)
   {
       printf("PatchCorrespondenceGen::readPatches: not enough storage for patches\n");
       readOK=false;
   }

   int maxIDReadIn=-1;
   for (int i=0; i<m_nPatches && readOK; i++)
   {
       // read the id and world coordinates of this patch
       double wx, wy, wz;
       int id;
       if (readIDs) fread((void *)&id, sizeof(id), 1, fp);
       else id=i;  // if not reading IDs, give them ID equal to index
       if (maxIDReadIn < id) maxIDReadIn=id;  // note the biggest ID seen as we need this for knowing the next ID to assign when craeting new patches

       fread((void *)&wx, sizeof(wx), 1, fp);
       fread((void *)&wy, sizeof(wy), 1, fp);
       fread((void *)&wz, sizeof(wz), 1, fp);
       readOK = m_texturePatches[i]->readPatch(fp);     // read all data that is specific to this patch

       if (m_texturePatches[i]->getExpiryTime() < 0)
       {
      m_texturePatches[i]->setExpiryTime(-1);
       }

       int fixedAxis=m_texturePatches[i]->getFixedAxis();  // fixed axis is stored with the patch but need to know for constraints to PoseEstPanTiltHead
       //int id = m_PoseEst->getPoint(i, &wx, &wy, &wz);   // (this is abs pos in world with cam at 0,0,0)
       // add the world patch coords to the PoseEstPanTiltHead object
       // NB point now added to object with ID 1 greater than patch index (ie. one point per object), leaving object 0 for points
       // that define the world ref frame
       bool fixPosX=true;
       bool fixPosY=true;
       bool fixPosZ=true;
       if (fixedAxis==1) {fixPosY=false; fixPosZ=false; }
       if (fixedAxis==2) {fixPosZ=false; fixPosX=false; }
       if (fixedAxis==3) {fixPosX=false; fixPosY=false; }

       // add this point: as no points are added yet, the ref frame for this point will be set to that of the ref object, ie. minus the cam pos
       // so that its position w.r.t. the camera at the origin will be givenPos-camPos, or equivalently givenPos+refObjPos
       int pointIndex = m_PoseEst->addPoint(wx, wy, wz, i, fixPosX, fixPosY, fixPosZ, id);   // pos w.r.t. camera at origin
       if (pointIndex <0) // should never happen as will be caught by test earlier
       {
      printf("PatchCorrespondenceGen::readPatches: not enough storage for points\n");
      readOK=false;
       }
   }
   // use maxIDReadIn to set the next ID for created points:
   m_PoseEst->setNextFreeID(maxIDReadIn+1);
   if (readOK) printf("PatchCorrespondenceGen::readPatches: successfully read data for %d patches; next free ID=%d\n", m_nPatches, maxIDReadIn+1);
   return (readOK);
    }
}

void PatchCorrespondenceGen::drawSuggestedFeatures(unsigned char *image, int incX, int incY, int lenX, int lenY)
{
    double imX, imY;
    double length = 40;


    for (int j = 0;j<(int)printMatchX.size();j++)
    {

//    length = double(printMatchScore[j]);
   imX = double(printMatchX[j]);
   imY = double(printMatchY[j]);
   FittedLine hLine(imX-length, imY, imX+length, imY);
   hLine.drawLine(image, incX, incY, lenX, lenY);
    }
}


void PatchCorrespondenceGen::drawCheckCorrespondenceLine(unsigned char *image, int incX, int incY, int lenX, int lenY)
{
    double imX, imY;
    double length;

    imX = 20.0;
    imY = 20.0;
    length = printMatch*700;

    FittedLine hLine(imX, imY, imX+length, imY);
    hLine.drawLine(image, incX, incY, lenX, lenY);
    FittedLine hLine2(imX, imY+1, imX+length, imY);
    hLine.drawLine(image, incX, incY, lenX, lenY);
    FittedLine hLine3(imX, imY+2, imX+length, imY);
    hLine.drawLine(image, incX, incY, lenX, lenY);
    FittedLine hLine4(imX, imY+3, imX+length, imY);
    hLine.drawLine(image, incX, incY, lenX, lenY);
    FittedLine hLine5(imX, imY+4, imX+length, imY);
    hLine.drawLine(image, incX, incY, lenX, lenY);
    FittedLine hLine6(imX, imY+5, imX+length, imY);
    hLine.drawLine(image, incX, incY, lenX, lenY);

}



void PatchCorrespondenceGen::addPermanentPatchesClassifier()
{
    if(!(rf))
    {
   printf("\nError. No classifier has been constructed\n");
   return;
    }


    //printf("PatchCorrespondenceGen::addPermanentPatchesClassifier: %d patches...\n",m_nPatches);
    int adds = 0;
    for (int i=0; i<m_nPatches; i++)
    {
   if (m_texturePatches[i]->getExpiryTime() < 0)
   {
       m_texturePatches[i]->setExpiryTime(-1);

       if((m_patchVisible[i])&&(m_PoseEst->pointWasAnInlier(i)))
       {
      double imX, imY;
      double x2, y2;


      int uniqueID = m_PoseEst->getPointObservation(i, &x2, &y2);


//    m_PoseEst->projectIntoImageObjectPoint(0, &imX, &imY, true, i);

      //     printf("(%f, %f), (%f, %f)\n", x2, y2, imX, imY);

//printf("%d, %d, (%f, %f) %d", uniqueID, i, x2, y2, m_texturePatches[i]->getExpiryTime());

//        imX = m_PoseEst->getImagePoints(i,0)[0][0];
      //      imY = m_PoseEst->getImagePoints(i,0)[0][1];
      if (x2 > 0)
      {

          int result = addToClassifier(int(x2), int(y2), uniqueID);
          if (result == 1)
          {

         adds++;

          }
          else if (result == 0)
          {
         m_texturePatches[i]->setExpiryTime(-2);
          }
      }
      else
      {
      }
       }  // if negative expiry time
   }//if visible

    } // for i=number of patches

}


int PatchCorrespondenceGen::findClassifierCorrespondences(int mode, int maxNewFeaturesToFind, int cycle, int subsample, double initialiseScale)
{
    int nFound = 0;



    double ratioLine = ((double)m_lenY*0.75) / (double)m_lenX;

    int lineY = m_lenY;



    int m_lenXNew = m_lenX;
    int xMin = 0;
    int yMin = 0;

    if (cycle < 1)
    {
   match_array_index = 0;
   match_array.resize(0);
   match_array.resize(150);
   std::map<int, int> indexlistMapNew;
   std::map<int, double> scorelistMapNew;
   indexlistMap = indexlistMapNew;
   scorelistMap = scorelistMapNew;

    }
    if (cycle >=0 )
    {
   maxNewFeaturesToFind = maxNewFeaturesToFind/2;
   m_lenXNew = m_lenX/2;

   xMin = (m_lenXNew * cycle);

    }

    int maxToAdd = 30;//30; //was 20


    int correspondenciesUsed = 0;

    int matchscore = 0;
    int matchscoretotal = 0;
    int expectedPatches = 0;

    printMatchX.resize(0);
    printMatchY.resize(0);
    printMatchScore.resize(0);


    /* timeval startTime, endTime;
       gettimeofday(&startTime, 0);*/




    int featuresFound2 = 0;
//  printf("%d %d |%d|\n", m_featureFindSsX, m_featureFindSsY, m_minDist);
    //------- Matching with a new feature finder
    featuresFound2 = m_KLTTracker->findGoodFeatures(
   m_image[m_cur][0] + xMin * m_incX[0] + yMin * m_incY[0], //m_image[m_cur][0],   // top-left pixel of region to look in CODE TO DO OFFSET: m_image[m_cur][0] + xMin * m_incX[0] + yMin * m_incY[0],   // top-left pixel of region to look in
   m_lenXNew, m_lenY,         // dims of region to search
   m_incX[0], m_incY[0],         // arrays giving x & y increments for each level (current image)
   m_halfWidX, m_halfWidY,             // width around each pixel to sum (so ap is 2*halfWid+1 in both x & y)
   m_featureFindSsX+subsample, m_featureFindSsY, // step size when searching for maxima
   m_arrayLenForFeatureFind,             // length of arrays available for use- will limit number to maxNewFeaturesToFind later
   m_FeatureTempX, m_FeatureTempY, m_FeatureTempVal,          // arrays to put coords of up to maxFeaturesToFind in
   m_minFeatureVal,
   m_minFeatureValBigEigen,
   15//m_minDist
   );
    
    if (featuresFound2 > maxNewFeaturesToFind) featuresFound2=maxNewFeaturesToFind;  // limit number to what we really want

    int foundandmatched = 0;
    int foundandmismatched = 0;

    /* gettimeofday(&endTime, 0);
       double locationTime = ( (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000000.0);
       printf("D: %f\n", locationTime);
       startTime = endTime;*/



    for (int i=0;((i<featuresFound2)&&(i<maxNewFeaturesToFind)); i++)
    {
   double score;

   //gettimeofday(&startTime, 0);


   lineY = (int)(((double)m_FeatureTempX[i]+(double)xMin)*ratioLine);

   int res1 = -1;

//   printf("%d vs %d\n", lineY, (m_FeatureTempY[i]+yMin));
//   if (lineY < m_FeatureTempY[i]+yMin)
//   {
       res1 = rf->findFeature(m_FeatureTempX[i]+xMin, m_FeatureTempY[i]+yMin, *id, &score, initialiseScale); //x coord, y coord, (offsets are ignored) image.
//       printf("IN! %d\n", res1);
//   }

   /*gettimeofday(&endTime, 0);
     locationTime = ( (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000000.0);
     printf("%f ", locationTime);
     startTime = endTime;*/


   if ((mode == 1)&&(res1 != -1))
   {
       for (int j=0; j<m_nPatches; j++)
       {
      double imX, imY;

      if (m_texturePatches[j]->getExpiryTime() < 0)
      {
          imX = m_PoseEst->getImagePoints(j,0)[0][0];
          imY = m_PoseEst->getImagePoints(j,0)[0][1];

          int hitsize=2;

          if ((m_FeatureTempX[i]+xMin>(int(imX)-hitsize))&&(m_FeatureTempX[i]+xMin<(int(imX)+hitsize))&&(m_FeatureTempY[i]+yMin>(int(imY)-hitsize))&&(m_FeatureTempY[i]+yMin<(int(imY)+hitsize)))
          {

         foundandmatched++;
         break;
          }
      }
       }
   }

   int temp;


   if ((res1 != -1))
   {

       if (mode == 1)
       {
      printMatchX.push_back(m_FeatureTempX[i]+xMin);
      printMatchY.push_back(m_FeatureTempY[i]+yMin);
       }

       //testing results

       if ((score > scorelistMap[res1])&&(res1 >=0))
       {

      if (!scorelistMap[res1])
      {
          nFound++;




          match_array[match_array_index].x = m_FeatureTempX[i]+xMin;
          match_array[match_array_index].y = m_FeatureTempY[i]+yMin;
          match_array[match_array_index].index = res1;
          match_array[match_array_index].prob = score;
          scorelistMap[res1] = score;
          indexlistMap[res1] = match_array_index;
          match_array_index++;

      }
      else
      {

          match_array[indexlistMap[res1]].x = m_FeatureTempX[i]+xMin;
          match_array[indexlistMap[res1]].y = m_FeatureTempY[i]+yMin;
          match_array[indexlistMap[res1]].index = res1;
          match_array[indexlistMap[res1]].prob = score;
          scorelistMap[res1] = score;
      }
       }

       correspondenciesUsed++;
   }
    }

    /*gettimeofday(&endTime, 0);
      locationTime = ( (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000000.0);
      printf("F: %f\n", locationTime);
      startTime = endTime;*/
    //------- Matching with a new feature finder


    //printf("matches so far: %d\n", match_array_index);


    //Stop here unless we have a complete collection of points.

    if ((cycle == 0))
    {
//    printf("No estimate this field\n");
   return -1;
    }



    //sort the the points
    quicksort(0, match_array_index);

    if (mode == 1)
    {


   for (int i=0; i<m_nPatches; i++)
   {
       if (m_texturePatches[i]->getExpiryTime() < 0)
       {
      expectedPatches++;
       }
   }

   int superscore=0;
   int notsuperscore=0;
   for (int j=0;((j<int(match_array_index))&&(j < maxToAdd));j++)
   {
       int temp = m_PoseEst->getPointIndex(match_array[j].index, getNPatches());

       for (int i=0; i<m_nPatches; i++)
       {
      double wx, wy, wz;
      double imX, imY;
      double x2, y2;

      if (m_texturePatches[i]->getExpiryTime() < 0)
      {
          imX = m_PoseEst->getImagePoints(i,0)[0][0];
          imY = m_PoseEst->getImagePoints(i,0)[0][1];
          int uniqueID = m_PoseEst->getPointObservation(i, &x2, &y2);

          if (temp == i)
          {

         if ((int(imX) > (match_array[j].x-2))&&(int(imY) > match_array[j].y-2)&&(int(imX) < (match_array[j].x+2))&&(int(imY) < match_array[j].y+2))
         {
             //printf("!");
             superscore++;
             break;
         }
         //else
         //{
         //   notsuperscore++;
         //}
          }
          //else
          //{
          //   notsuperscore++;
          //}

      }

       }
   }
   //printf("Score: %d/20\n", superscore);
   //printf("Found: %d out of %d with %d in total %d\n", foundandmatched,expectedPatches,featuresFound2, foundandmismatched);
//      printf("%d %d\n", nFound, m_nPatches);
//      printf("%d %d\n", superscore, featuresFound2);
   printf("%d %f %d ", superscore, double(superscore)/double(expectedPatches), featuresFound2);
    }

    if (mode != 1) {
   m_PoseEst->clearAllPoints(); //If this is activated then the permanent features have their 2D coords wiped.

   for (int i=0;((i<match_array_index)&&(i < maxToAdd));i++)
   {
       int temp = m_PoseEst->getPointIndex(match_array[i].index, getNPatches());
       //printf("%d (%d, %d)\n", temp, match_array[i].x, match_array[i].y);
       if (temp > -1)
       {
      m_PoseEst->addPointObservation(temp, double(match_array[i].x), double(match_array[i].y));
       }
       else
       {
//            printf("Unknown object...\n");
       }
   }
    }

//  rf->printBalance();


/*      gettimeofday(&endTime, 0);
   locationTime = ( (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000000.0);
   printf("S: %f\n", locationTime);*/

    return nFound;
}

double PatchCorrespondenceGen::checkCorrespondences(bool allowFreeMove, double initialiseScale, bool ignoreOutliers)
{

    if(!(rf))
    {
   printf("\nError. No classifier has been constructed\n");
   return 0.0;
    }



    allowFreeMove = false;


    /*
      A SUGGESTION

      Work to be done examining the attributes containes within the features. Field of View for example. All features should have the same fov at any point? So it a feature is coming back with the incorrect fov this may be significant?????????

      Also, ignore features at significantly differnt depths so that we aren't flooded with features after a zoom.


      2nd Suggestion. Would periodic initialisations be help ful when there are very low scores. I ~50%. this may prevent the view from being drifited (rotation?) a small amount. (this can be simulated with a hand) -
      This should be in a siutation where we would still be confident of a correct result, with no erronoes initilisation. ie, 50% of a large number of points, so we stall have ten or so.
      I think this may not occurs when learning is off - check this. May still have worth.
    */

    int totalcds = m_nPatches;

//-------- Matching with existing features

    int matchscore = 0;
    int matchscoretotal = 0;
    int expectedPatches = 0;

    int missedPatches = 0;

    for (int i=0; i<m_nPatches; i++)
    {
   double wx, wy, wz;
   double imX, imY;
   double x2, y2;


   if((m_patchVisible[i])&&(m_PoseEst->pointWasAnInlier(i)||!ignoreOutliers))
   {
       if (m_texturePatches[i]->getExpiryTime() < 0)
       {
      m_texturePatches[i]->setExpiryTime(-1);

      //imX = m_PoseEst->getImagePoints(i,0)[0][0];
      //imY = m_PoseEst->getImagePoints(i,0)[0][1];

      int uniqueID = m_PoseEst->getPointObservation(i, &x2, &y2);

      m_PoseEst->projectIntoImageObjectPoint(0, &imX, &imY, true, i);



//       printf("(%f, %f), (%f, %f)\n", x2, y2, imX, imY);


      //expectedPatches++;
      //It may be better to use this as the denominator rather than matchscoretotal
      //matchscoretotal ignores those features that the tracker considers to be lost.
      //It might be a suggestion to use the coordinates of the features that are ignored.




      if (imX > 0)
      {
          double score;
          int res1;
          if (x2 > 0)
          {
         expectedPatches++;
         res1 = rf->findFeature(int(x2), int(y2), *id, &score, initialiseScale); //x coord, y coord, (offsets are ignored) image.
          }
          else
          {
         res1 = rf->findFeature(int(imX), int(imY), *id, &score, initialiseScale); //x coord, y coord, (offsets are ignored) image.
          }

          if (uniqueID == res1)
          {
         //printf("Score: %f\n", score);
         matchscore++;
         matchscoretotal++;
         m_texturePatches[i]->setExpiryTime(-2);
          }
          else
          {
         //printf("Missed: %d, %d\n", uniqueID, res1);
         //printf("adding: %d (%d, %d) - %d (%d, %d)\n", i, imX, imY, uniqueID, int(x2), int(y2));
         matchscoretotal++;

          }
      }
      else
      {

          //printf("Odd: %d\n", i);
          //printf("%f %f\n", x2, y2);
          //printf("%f %f\n", imX, imY);
      }
       }
       else
       {
      missedPatches++;
       }
   }//visible inlier

    }

    double scoreVal;

    if (matchscoretotal == 0)
    {
   scoreVal = 0;
    }
    else
    {
//    scoreVal = double(matchscore)/double(matchscoretotal);

   //printf("%d %d %d (%d)\n", matchscore, matchscoretotal, expectedPatches, missedPatches);
   if (matchscoretotal < 10)
   {matchscoretotal = 10;}
   else if (matchscoretotal > 30)
   {matchscoretotal = 30;}
   scoreVal = double(matchscore)/double(matchscoretotal);

/*    if (matchscore > 0)
      {
      scoreVal = scoreVal*(1+((matchscore)/10));

      }*/
    }

    /*printf("All patches: %d, expected patches: %d, score: %d/%d --- %f\n", m_nPatches, expectedPatches,  matchscore,matchscoretotal, scoreVal);

    printMatch = scoreVal;

    if (((expectedPatches > (matchscoretotal+8))||((matchscoretotal > 8)&&(matchscore >= matchscoretotal-1))))//OR when matchscore is high and matchscoretotal???
    {
    //object obscuring points
    freeMove = false;
    }

    if (scoreVal < 0.1)
    {
    if (freeMove)
    {
    printf("freeMove!!!!\n");
    if (allowFreeMove)
    {
    return 1;
    }
    }
    }
    else
    {
    if (expectedPatches > (matchscoretotal+1))
    {
    //object obscuring points
    freeMove = false;
    }
    else
    {
    //leaving known area
    if (allowFreeMove){printf("freeMove\n");}
    freeMove = true;
    }
    }*/

    if (scoreVal > 1.0)
    {
   scoreVal = 1.0;
    }

    return scoreVal;
}

double PatchCorrespondenceGen::testClassifierInitialisation(double maxInlierDist, double minInlierFraction, int maxTries,
                         int nInitialPoints, int maxNewFeaturesToFind, int cycle, int subsample, double initialiseScale)
{

    if(!(rf))
    {
   printf("\nError. No classifier has been constructed\n");
   return 0;
    }

    timeval startTime, endTime;

    gettimeofday(&startTime, 0);


    int nFound = this->findClassifierCorrespondences(1, maxNewFeaturesToFind, cycle, subsample, initialiseScale);
    //printf("nFound: %d\n\n", nFound);

    gettimeofday(&endTime, 0);
    double initialiseTime = ( (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000000.0);
    printf("%f\n", initialiseTime);

    return 0;
}



int PatchCorrespondenceGen::addToClassifier(int xx, int yy, int uniqueID)
{
    if(!(rf))
    {
   printf("\nError. No classifier has been constructed\n");
   return -2;
    }

    bool addit = false;


    //double x2, y2;
    //int i = m_PoseEst->getPointObservation(uniqueID, &x2, &y2);


    if (!addit)
    {
   //Needs to be a test to prevent the points being added. Examples: The camera has moved, and/or the RANSAC fails to match universally.


   if ((xx > 0)&&(yy > 0))
   {
       double nothing; //NEED TO ALTER HOW THIS FUNTION IS CALLED SO i CAN REMOVE THE DOUBLE AT THE END

       /*struct timeval tv;
         long int curTime;
         long int preTime;
         long int difTime;
         gettimeofday(&tv, NULL);
         preTime = tv.tv_usec;*/

       //  printf("FF: ");
       int res = rf->findFeature(xx, yy, *id, &nothing); //x coord, y coord, (offsets are ignored) image.
       //printf(" :FF\n");



       /*gettimeofday(&tv, NULL);
         curTime = tv.tv_usec;
         //   printf("c: %d\n", curTime);
         difTime = (curTime-preTime);
         if (difTime < 0)
         {
         difTime = 1000000 + difTime;
         }*/

/*
  printf("%d vs %d\n", res, uniqueID);
*/
       if (res == uniqueID)
       {
      addit = false;
       }
       else if (res == -2)
       {
      return -1;
       }
       else
       {
      addit = true;
       }

       if (addit == true)
       {
      /*gettimeofday(&tv, NULL);
        preTime = tv.tv_usec;*/

//        printf("AF: ");
      rf->addFeature(xx, yy, *id, uniqueID);
      //      printf(" :AF\n");
/*
  printf(" :AF\n");
*/
      /*gettimeofday(&tv, NULL);
        curTime = tv.tv_usec;
        //   printf("c: %d\n", curTime);
        difTime = (curTime-preTime);
        if (difTime < 0)
        {
        difTime = 1000000 + difTime;
        }*/
      return 1;
       }
   }
   else
   {
       return -1;
   }
    }
    return 0;
}



void PatchCorrespondenceGen::buildClassifier()
{
    rf->buildForest();
    printf("Classifier built\n");
}

void PatchCorrespondenceGen::optimiseClassifier()
{
    //rf->shrinkForest();
    rf->shrinkForest(*m_PoseEst);
}



bool PatchCorrespondenceGen::writeClassifier(char *fileName)
{

    FILE *fp;

    //#define TREE_FILE_VERSION_STRING "Tree description v1.1\n"  // write at start of file & check

    if ( (fp = fopen(fileName, "w")) == NULL)
    {
   printf("Can't open tree file %s for writing\n",fileName);
   return(false);
    }


//  FILE *fp2;
//  if ( (fp2 = fopen("treeDebug.xml", "w")) == NULL)
//  {
//      printf("Can't open tree file %s for writing\n",fileName);
//      return(false);
//  }


    /**/    printf("PatchCorrespondenceGen::writeClassifier writing %s\n",fileName);

//   rf->writeForest(fp, fp2, true);
    rf->writeForest(fp);

    fclose (fp);
//   fclose (fp2);

    return(true);
}






bool PatchCorrespondenceGen::readClassifier(char *fileName)
{

    FILE *fp;
    if ( (fp = fopen(fileName, "r")) == NULL)
    {
   printf("Can't open patch file %s for reading\n",fileName);
   return(false);
    }


    /**/printf("PatchCorrespondenceGen::readClassifier reading %s\n",fileName);

    rf->readForest(fp);

    fclose (fp);



    return (true);

}

void PatchCorrespondenceGen::updatePatchFocalLength()
{
    for (int i=0; i<m_nPatches; i++)
    {
      int im = m_PoseEst->getFirstImageWithObjectVisible(i);
      if (im >= 0) // check it was seen - should always have been seen if deleteUnseenpatches() was called before fullSOlution
	{
	  double prevFocLen;
	  m_PoseEst->getCamFocalLengthBeforeFullSolution(&prevFocLen, im);
	  if (prevFocLen > 0.0)
	    {
	      double curFocLen;
	      m_PoseEst->getCamFocalLength(&curFocLen, im);
	      double focLenChangeFactor = curFocLen / prevFocLen;
	      double newFocLen = m_texturePatches[i]->getFocalLength() * focLenChangeFactor;
	      m_texturePatches[i]->setFocalLength(newFocLen);
	      printf("PatchCorrespondenceGen::updatePatchFocalLength: patch %d foc len scaled by %6.2f\n", i, focLenChangeFactor);
	    }
	  else printf("PatchCorrespondenceGen::updatePatchFocalLength: patch %d cannot be updated as there was no previos foc len stored\n", i);
	}
      else printf("PatchCorrespondenceGen::updatePatchFocalLength: patch %d cannot be updated as it was not seen in any stored image\n", i);
    }

}


void PatchCorrespondenceGen::updatePatchReliabilityMeasure()
{
    // designed to be called after calcSolutionRANSAC and before outlier deletion or creation of new patches
    for (int i=0; i<m_nPatches; i++)
    {
   if ( m_patchVisible[i] )  // m_patchVisible is set for each patch by findCorrespondencies
   {
       m_texturePatches[i]->incrementTimesSeen();
       if (m_PoseEst->pointWasAnInlier(i)) m_texturePatches[i]->incrementTimesInlier();
   }
    }
}

void PatchCorrespondenceGen::resetPatchReliabilityMeasure()
{
    // designed to be called after calcSolutionRANSAC and before outlier deletion or creation of new patches
    for (int i=0; i<m_nPatches; i++)
    {
   m_texturePatches[i]->setTimesSeen(0);
   m_texturePatches[i]->setTimesInlier(0);
    }
}

int PatchCorrespondenceGen::discardStoredPatchesUsingTriStripRegion()
{
  // perform test for each stored camera.  Could do this with one really wide-angle camera, or project from camera pos onto
  // x, y or Z plane, for example, but this approach makes use of existing methods and will do the job, as all features
  // that are stored (after having their positions refined) must be visible in at least one stored image

  int patchesDiscarded=0;
  if ( m_useIgnoreTriStrips )
    {
      // loop for all patches (need to do patches outside so we have results from all cameras for a patch before deciding if to keep)
      for (int p=0; p<m_nPatches; p++)
   {
     bool inTriangle=false;  // not in any triangle so far: will set true if appears in any at all
     for (int cam=0; cam<m_PoseEst->getNImages(); cam++)
       {
         // Project any triangle strip verticess into the image so we can test for new features falling in these:
         for (int n=0; n<m_nTriStrips; n++)
      {
        for (int i=0; i<m_nTriPoints[n]; i++)
          {
            // project this point into the image, allowing it to land outside but checking for it being behind the camera
            m_projTriPointUsable[n][i] =
         m_PoseEst->projectIntoImageArbitraryPoint(m_triPoints[n][3*i], m_triPoints[n][3*i+1], m_triPoints[n][3*i+2],
                          &m_projTri[n][2*i], &m_projTri[n][2*i+1],
                          false,  // don't bother about applying distortion, esp as could give weird results for points landing way outside the image
                          -1, cam, false);  // default object, this camera, don't trap points outside image
          }  // i=point in tri
      }  // loop for n=0..m_nTriPoints-1

         double imX, imY;
         if (m_PoseEst->projectIntoImageObjectPoint(0, &imX, &imY, false, p, cam))  // pth patch is 0th point on object i (no distortion: wasnut used for triangle vertices)
      {
        // check point does (not) lie within the region(s) defined by the triangle strips
        for (int n=0; n<m_nTriStrips && !inTriangle; n++)
          {
            for (int i=2; i<m_nTriPoints[n] && !inTriangle; i++)  // start with the 3rd point in the list and consider triangle from this and two previous points
         {
           // if this vertex and previous two were all in front of the camera, consider this triangle:
           //printf("  Tri strip %d, starting with point %d:\n", n, i);
           if (m_projTriPointUsable[n][i] &&  m_projTriPointUsable[n][i-1] &&  m_projTriPointUsable[n][i-2])
             {
               inTriangle = inTriangle || pointInTriangle(imX, imY, &m_projTri[n][2*i], &m_projTri[n][2*(i-1)],&m_projTri[n][2*(i-2)]);
               //if (inTriangle) printf("    point inside\n"); else printf("     point outside\n");
             }
           //else printf("    One or more vertices was behind camera, so triangle not usable\n");
         }
          }


      }  // if visible in this camera
       } //cam=camera
     bool discard =  (inTriangle && m_ignoreWithinTriangles) || (!inTriangle && !m_ignoreWithinTriangles);

     if (discard)
       {
         //printf("PatchCorrespondenceGen::discardStoredPatchesUsingTriStripRegion: Discarding patch %d\n", p);
         this->deletePatch(p);
         patchesDiscarded++;
         p--;  // go back and try the patch that will have been put in the place of the one just deleted
       }
   } // p=patch
    }
  return(patchesDiscarded);
}






bool PatchCorrespondenceGen::detectNonCameraMotionReverse(unsigned char threshold, int nProjX, int nProjY) {
  //printf("Starting PatchCorrespondenceGen::detectNonCameraMotion...\n");
  if(m_motionMaskResolution<0) {
    fprintf(stdout,"PatchCorrespondenceGen::detectNonCameraMotion Motion mask cannot be computed either because there are not enough images avilable or the mask array has not been initialised.\n");
    return false;
  }
  // set up camera and image indeces plus number of images needed depending on whethwe we're operating over a period of 1 or 2 ims

  int pastCameraIndex, pastImageIndex, imagesNeeded;
  if (m_measureMotionMaskAcrossFramePeriod)
    {
      pastCameraIndex = m_PoseEst->getPrevButOneImageIndex();  // for passing to PoseEStPanTiltHead to select desired cam params
      pastImageIndex = m_prevButOne;     // for indexing ring buffer of images
      imagesNeeded=3;
    }
  else
    {
      pastCameraIndex = m_PoseEst->getPrevImageIndex();
      pastImageIndex = m_prev;
      imagesNeeded=2;
    }

  //printf("m_nImagesLoaded=%d  m_firstGOodImage=%d  imagesNeeded=%d\n", m_nImagesLoaded, m_firstGoodImage, imagesNeeded);
  if (m_nImagesLoaded - m_firstGoodImage + 1 >= imagesNeeded)
    {
      // dims of subsampled image
      int ssFactor = 1 << m_motionMaskResolution;
      int lenX = m_lenX >> m_motionMaskResolution;
      int lenY = m_lenY >> m_motionMaskResolution;
      // declare iterator integers and array index integer
      int index, i, j, i2, j2, xiPix, yiPix;
      double xfMap, yfMap, xfPix, yfPix;
      unsigned char *pix_ptr;
      int pix_diff;

      //bool applyDistortion=false;  // TOOD: make selectable externally, or at least based on whether there is any distortion!
      bool applyDistortion=true;  // TOOD: make selectable externally, or at least based on whether there is any distortion!

      //printf("PatchCorrespondenceGen::detectNonCameraMotion:\n");
      //double pan, tilt, roll, foclen;

      //m_PoseEst->getCamRotPTR(&pan, &tilt, &roll,  m_PoseEst->getPrevImageIndex());
      //m_PoseEst->getCamFocalLength(&foclen, m_PoseEst->getPrevImageIndex());
      //printf("  previous image at index %4d has pan=%6.2f tilt=%6.2f roll=%6.2f foclen=%6.2f\n", m_PoseEst->getPrevImageIndex(), pan, tilt, roll, foclen*1000.0);
      //m_PoseEst->getCamRotPTR(&pan, &tilt, &roll,   m_PoseEst->getNImages()-1);
      //m_PoseEst->getCamFocalLength(&foclen, m_PoseEst->getNImages()-1);
      //printf("  current image at index %4d has pan=%6.2f tilt=%6.2f roll=%6.2f foclen=%6.2f\n",  m_PoseEst->getNImages()-1, pan, tilt, roll, foclen*1000.0);

      // instead of having fixed spacing between points that we then bilearly interpolate between, make loop start and end on pixels at image edge,
      // and adjust spacing of points as we go
      if (nProjX < 2) nProjX=2; if (nProjX > lenX) nProjX=lenX;         // ensure is in range 2..lenX/Y inclusive
      if (nProjY < 2) nProjY=2; if (nProjY > lenY) nProjY=lenY;
      int prevj=-1, previ=-1;

      // get projection for spaced points
      // i,j are coords in mask image we're going to generate, which is at level m_motionMaskResolution of the image pyramid
      //for (j = 0; j < lenY; j += proj_spaceY) {
      for (int py=0; py<nProjY; py++) {
   j=(int) ( (double)(lenY-1) * (double)py/(double)(nProjY-1) + 0.5);  // nearest sample in this level of im pyramid to the py'th sample point
   //for (i = 0; i < lenX; i += proj_spaceX) {
   for (int px=0; px<nProjX; px++) {
     i=(int) ( (double)(lenX-1) * (double)px/(double)(nProjX-1) + 0.5);

     index = j*lenX + i;
     //printf("projecting point at %d %d wich is the %d,%d th projection point with index=%d  lenx=%d leny=%d\n", i, j, px, py, index, lenX, lenY);
     // project line of sight from pixel in current frame into world space.
     //obtaining a unit vector from the camera's origin in world space.
     // then project point at the end of the unit vector, from world space back into
     // camera image of previous frameage of previous frame

     if (!m_PoseEst->projectBetweenIms(i << m_motionMaskResolution, j << m_motionMaskResolution,
                   &m_interFrameMappingX[index], &m_interFrameMappingY[index],
                   pastCameraIndex, m_PoseEst->getNImages()-1, applyDistortion, false)) // from previous image (as stored by addNewImage) and latest image, apply distortion?, don't trap outside image
       { printf("PoseEst->projectBetweenIms returned false - cameras out of range - programming error!!\n"); exit(-1); }

     // scale mappings for correct image level
     m_interFrameMappingX[index] /= ssFactor;
     m_interFrameMappingY[index] /= ssFactor;

     if (px > 0 && py > 0) {
       // bilinear interpolation to fill holes between mapped points
       int indexBR = index;
       int indexBL = j*lenX + previ;
       int indexTR = prevj*lenX + i;
       int indexTL = prevj*lenX + previ;
       // check if any of the corners of this block are outside of the previous image.
       // if so discard this block (could extrapolate but probably not worth it)
       //printf("Doing bilinear interp\n");
       int this_proj_spaceX = i - previ;  // how big the step was across the block we're interpolating in
       int this_proj_spaceY = j - prevj;
       for (j2 = 0; j2 <= this_proj_spaceY; j2++) {
         yfMap = ((double) j2) / ((double) this_proj_spaceY);
         int proj_index = (prevj+j2)*lenX + previ;  // index of first pixel to write result into
         for (i2 = 0; i2 <= this_proj_spaceX; i2++) {
      //printf(" Bilin interp for point %d,%d writing to projindex = %d\n", previ+i2, prevj+j2, proj_index);
      xfMap = ((double) i2) / ((double) this_proj_spaceX);
      // get interpolated mappings
      m_interFrameMappingX[proj_index] =
        m_interFrameMappingX[indexTL] * (1 - xfMap) * (1 - yfMap) +
        m_interFrameMappingX[indexTR] * xfMap * (1 - yfMap) +
        m_interFrameMappingX[indexBL] * (1 - xfMap) * yfMap +
        m_interFrameMappingX[indexBR] * xfMap * yfMap;
      m_interFrameMappingY[proj_index] =
        m_interFrameMappingY[indexTL] * (1 - xfMap) * (1 - yfMap) +
        m_interFrameMappingY[indexTR] * xfMap * (1 - yfMap) +
        m_interFrameMappingY[indexBL] * (1 - xfMap) * yfMap +
        m_interFrameMappingY[indexBR] * xfMap * yfMap;
      // Check that this point is in the range 0..lenX/Y-2
      // (must be less than or equal to len-2 (NOT -1) as we use the samples below AND above when doing the
      // bilinear interp
      xiPix = (int) m_interFrameMappingX[proj_index];
      yiPix = (int) m_interFrameMappingY[proj_index];
      if (xiPix >= 0 && xiPix <= lenX-2 &&
          yiPix >= 0 && yiPix <= lenY-2)
        {
          // now get value of pixel in both images
          pix_diff = (int) m_image[pastImageIndex][m_motionMaskResolution][proj_index];  // pixel in current image
          // get interpolated pixel value in previous frame
          xfPix = m_interFrameMappingX[proj_index] - xiPix;  // fraction of pixel position beyond rounded-down location
          yfPix = m_interFrameMappingY[proj_index] - yiPix;
          //printf("input sample xiPix,yiPix = %d %d at index %d and will write to index %d\n", xiPix, yiPix, xiPix + yiPix * lenX, proj_index);
          //pix_ptr = &m_image[m_cur][m_motionMaskResolution][xiPix + yiPix * lenX]; // cur/prev, pyramid level, pixel offset
          pix_ptr = &m_image[m_cur][m_motionMaskResolution][xiPix + yiPix * lenX]; // cur/prev, pyramid level, pixel offset
          pix_diff -= (int)((double)   *(pix_ptr) * (1 - xfPix) * (1 - yfPix)
                  + (double) *(pix_ptr + 1) * xfPix * (1 - yfPix)
                  + (double) *(pix_ptr + lenX) * (1 - xfPix) * yfPix
                  + (double) *(pix_ptr + 1 + lenX) * xfPix * yfPix);
          // compute threshold for mask and mean val.
          m_motionMaskReverse[proj_index++] = abs(pix_diff)>threshold;
          //m_motionMaskReverse[proj_index++] = (m_image[pastImageIndex][m_motionMaskResolution][proj_index] > 128);  // TEST: show prev image
          //m_motionMaskReverse[proj_index++] = (m_image[m_cur][m_motionMaskResolution][proj_index] > 128);  // TEST: show current image with no shift
          //m_motionMaskReverse[proj_index++] = ( *(pix_ptr) > 128);  // TEST: show current image with (almost) newarest pixel offsey
          //printf("Written result\n");
        }
      else // point outside image
        m_motionMaskReverse[proj_index++] = false;
         }  // for i2
       }  // for j2
       //printf("Done bilin interp\n");
     }  // if px>0 and py>0 so doing bilin interp
     previ=i;  // x coord of point just made
   }  // for px
   prevj=j;  // y coord of point just made
      }  // for py
      //printf("Finished PatchCorrespondenceGen::detectNonCameraMotion\n");
      m_gotValidCamMotionMask = true;
      return true;
    }  // if at least 3 images loaded
  else
    {
      m_gotValidCamMotionMask = false;
      return false;
    }  // if at least 3 images loaded or not
}












bool PatchCorrespondenceGen::detectNonCameraMotion(unsigned char threshold, int nProjX, int nProjY) {

  //printf("Starting PatchCorrespondenceGen::detectNonCameraMotion...\n");
  if(m_motionMaskResolution<0) {
    fprintf(stdout,"PatchCorrespondenceGen::detectNonCameraMotion Motion mask cannot be computed either because there are not enough images avilable or the mask array has not been initialised.\n");
    return false;
  }
  // set up camera and image indeces plus number of images needed depending on whethwe we're operating over a period of 1 or 2 ims

  int pastCameraIndex, pastImageIndex, imagesNeeded;
  if (m_measureMotionMaskAcrossFramePeriod)
    {
      pastCameraIndex = m_PoseEst->getPrevButOneImageIndex();  // for passing to PoseEStPanTiltHead to select desired cam params
      pastImageIndex = m_prevButOne;     // for indexing ring buffer of images
      imagesNeeded=3;
    }
  else
    {
      pastCameraIndex = m_PoseEst->getPrevImageIndex();
      pastImageIndex = m_prev;
      imagesNeeded=2;
    }
  //printf("m_nImagesLoaded=%d  m_firstGOodImage=%d  imagesNeeded=%d\n", m_nImagesLoaded, m_firstGoodImage, imagesNeeded);
  if (m_nImagesLoaded - m_firstGoodImage + 1 >= imagesNeeded)
    {
      // dims of subsampled image
      int ssFactor = 1 << m_motionMaskResolution;
      int lenX = m_lenX >> m_motionMaskResolution;
      int lenY = m_lenY >> m_motionMaskResolution;
      // declare iterator integers and array index integer
      int index, i, j, i2, j2, xiPix, yiPix;
      double xfMap, yfMap, xfPix, yfPix;
      unsigned char *pix_ptr;
      int pix_diff;

      //bool applyDistortion=false;  // TOOD: make selectable externally, or at least based on whether there is any distortion!
      bool applyDistortion=true;  // TOOD: make selectable externally, or at least based on whether there is any distortion!

      //printf("PatchCorrespondenceGen::detectNonCameraMotion:\n");
      //double pan, tilt, roll, foclen;

      //m_PoseEst->getCamRotPTR(&pan, &tilt, &roll,  m_PoseEst->getPrevImageIndex());
      //m_PoseEst->getCamFocalLength(&foclen, m_PoseEst->getPrevImageIndex());
      //printf("  previous image at index %4d has pan=%6.2f tilt=%6.2f roll=%6.2f foclen=%6.2f\n", m_PoseEst->getPrevImageIndex(), pan, tilt, roll, foclen*1000.0);
      //m_PoseEst->getCamRotPTR(&pan, &tilt, &roll,   m_PoseEst->getNImages()-1);
      //m_PoseEst->getCamFocalLength(&foclen, m_PoseEst->getNImages()-1);
      //printf("  current image at index %4d has pan=%6.2f tilt=%6.2f roll=%6.2f foclen=%6.2f\n",  m_PoseEst->getNImages()-1, pan, tilt, roll, foclen*1000.0);

      // instead of having fixed spacing between points that we then bilearly interpolate between, make loop start and end on pixels at image edge,
      // and adjust spacing of points as we go
      if (nProjX < 2) nProjX=2; if (nProjX > lenX) nProjX=lenX;         // ensure is in range 2..lenX/Y inclusive
      if (nProjY < 2) nProjY=2; if (nProjY > lenY) nProjY=lenY;
      int prevj=-1, previ=-1;

      // get projection for spaced points
      // i,j are coords in mask image we're going to generate, which is at level m_motionMaskResolution of the image pyramid
      //for (j = 0; j < lenY; j += proj_spaceY) {
      for (int py=0; py<nProjY; py++) {
   j=(int) ( (double)(lenY-1) * (double)py/(double)(nProjY-1) + 0.5);  // nearest sample in this level of im pyramid to the py'th sample point
   //for (i = 0; i < lenX; i += proj_spaceX) {
   for (int px=0; px<nProjX; px++) {
     i=(int) ( (double)(lenX-1) * (double)px/(double)(nProjX-1) + 0.5);

     index = j*lenX + i;
     //printf("projecting point at %d %d wich is the %d,%d th projection point with index=%d  lenx=%d leny=%d\n", i, j, px, py, index, lenX, lenY);
     // project line of sight from pixel in current frame into world space.
     //obtaining a unit vector from the camera's origin in world space.
     // then project point at the end of the unit vector, from world space back into
     // camera image of previous frameage of previous frame

     if (!m_PoseEst->projectBetweenIms(i << m_motionMaskResolution, j << m_motionMaskResolution,
                   &m_interFrameMappingX[index], &m_interFrameMappingY[index],
                   m_PoseEst->getNImages()-1, pastCameraIndex, applyDistortion, false)) // from previous image (as stored by addNewImage) and latest image, apply distortion?, don't trap outside image
       { printf("PoseEst->projectBetweenIms returned false - cameras out of range - programming error!!\n"); exit(-1); }

     // scale mappings for correct image level
     m_interFrameMappingX[index] /= ssFactor;
     m_interFrameMappingY[index] /= ssFactor;

     if (px > 0 && py > 0) {
       // bilinear interpolation to fill holes between mapped points
       int indexBR = index;
       int indexBL = j*lenX + previ;
       int indexTR = prevj*lenX + i;
       int indexTL = prevj*lenX + previ;
       // check if any of the corners of this block are outside of the previous image.
       // if so discard this block (could extrapolate but probably not worth it)
       //printf("Doing bilinear interp\n");
       int this_proj_spaceX = i - previ;  // how big the step was across the block we're interpolating in
       int this_proj_spaceY = j - prevj;
       for (j2 = 0; j2 <= this_proj_spaceY; j2++) {
         yfMap = ((double) j2) / ((double) this_proj_spaceY);
         int proj_index = (prevj+j2)*lenX + previ;  // index of first pixel to write result into

         for (i2 = 0; i2 <= this_proj_spaceX; i2++) {
      //printf(" Bilin interp for point %d,%d writing to projindex = %d\n", previ+i2, prevj+j2, proj_index);
      xfMap = ((double) i2) / ((double) this_proj_spaceX);
      // get interpolated mappings
      m_interFrameMappingX[proj_index] =
        m_interFrameMappingX[indexTL] * (1 - xfMap) * (1 - yfMap) +
        m_interFrameMappingX[indexTR] * xfMap * (1 - yfMap) +
        m_interFrameMappingX[indexBL] * (1 - xfMap) * yfMap +
        m_interFrameMappingX[indexBR] * xfMap * yfMap;
      m_interFrameMappingY[proj_index] =
        m_interFrameMappingY[indexTL] * (1 - xfMap) * (1 - yfMap) +
        m_interFrameMappingY[indexTR] * xfMap * (1 - yfMap) +
        m_interFrameMappingY[indexBL] * (1 - xfMap) * yfMap +
        m_interFrameMappingY[indexBR] * xfMap * yfMap;

      // Check that this point is in the range 0..lenX/Y-2
      // (must be less than or equal to len-2 (NOT -1) as we use the samples below AND above when doing the
      // bilinear interp
      xiPix = (int) m_interFrameMappingX[proj_index];
      yiPix = (int) m_interFrameMappingY[proj_index];
      if (xiPix >= 0 && xiPix <= lenX-2 &&
          yiPix >= 0 && yiPix <= lenY-2)
        {

          // now get value of pixel in both images
          pix_diff = (int) m_image[m_cur][m_motionMaskResolution][proj_index];  // pixel in current image
          // get interpolated pixel value in previous frame
          xfPix = m_interFrameMappingX[proj_index] - xiPix;  // fraction of pixel position beyond rounded-down location
          yfPix = m_interFrameMappingY[proj_index] - yiPix;
          //printf("input sample xiPix,yiPix = %d %d at index %d and will write to index %d\n", xiPix, yiPix, xiPix + yiPix * lenX, proj_index);
          //pix_ptr = &m_image[m_cur][m_motionMaskResolution][xiPix + yiPix * lenX]; // cur/prev, pyramid level, pixel offset
          pix_ptr = &m_image[pastImageIndex][m_motionMaskResolution][xiPix + yiPix * lenX]; // cur/prev, pyramid level, pixel offset
          pix_diff -= (int)((double)   *(pix_ptr) * (1 - xfPix) * (1 - yfPix)
                  + (double) *(pix_ptr + 1) * xfPix * (1 - yfPix)
                  + (double) *(pix_ptr + lenX) * (1 - xfPix) * yfPix
                  + (double) *(pix_ptr + 1 + lenX) * xfPix * yfPix);
          // compute threshold for mask and mean val.
          m_motionMask[proj_index++] = abs(pix_diff)>threshold;
          //m_motionMask[proj_index++] = (m_image[pastImageIndex][m_motionMaskResolution][proj_index] > 128);  // TEST: show prev image
          //m_motionMask[proj_index++] = (m_image[m_cur][m_motionMaskResolution][proj_index] > 128);  // TEST: show current image with no shift
          //m_motionMask[proj_index++] = ( *(pix_ptr) > 128);  // TEST: show current image with (almost) newarest pixel offsey
          //printf("Written result\n");
        }
      else // point outside image
        m_motionMask[proj_index++] = false;
         }  // for i2
       }  // for j2
       //printf("Done bilin interp\n");
     }  // if px>0 and py>0 so doing bilin interp
     previ=i;  // x coord of point just made
   }  // for px
   prevj=j;  // y coord of point just made
      }  // for py

      //printf("Finished PatchCorrespondenceGen::detectNonCameraMotion\n");
      m_gotValidCamMotionMask = true;
      return true;
    }  // if at least 3 images loaded
  else
    {
      m_gotValidCamMotionMask = false;
      return false;
    }  // if at least 3 images loaded or not
}

bool PatchCorrespondenceGen::getNonCameraMotion(int x, int y) {
  if (m_gotValidCamMotionMask)  // if we've generated one....
    {

      int masklenX = m_lenX >> m_motionMaskResolution;
      int masklenY = m_lenY >> m_motionMaskResolution;
      if(x<0 && y<0 && x>masklenX-1 && y>masklenY-1) {
   printf("PatchCorrespondenceGen::getNonCameraMotion - Called with x or y out of range (%d,%d) (%d,%d)\n",x,y,masklenX,masklenY);
   exit(-1);
      }
      return m_motionMask[x + masklenX * y];
    }
  else return false;
}

bool PatchCorrespondenceGen::upsampleAndDilateNonCameraMotionMaskReverse(unsigned char *image, int incX, int incY, int lenX, int lenY, int image_level,
                          bool dilate, unsigned int dilateKernelHalfWidth) {
  if (m_gotValidCamMotionMask)  // if we've generated one....
    {
        int masklenX = m_lenX >> m_motionMaskResolution;
   int masklenY = m_lenY >> m_motionMaskResolution;
   //printf("m_len %d,%d masklen %d,%d\n",m_lenX,m_lenY,masklenX,masklenY);
   // dims of upsampled image
   if(image_level > m_motionMaskResolution || lenX != m_lenX >> image_level || lenY != m_lenY >> image_level || (dilate && dilateKernelHalfWidth<1)) {
      fprintf(stdout,"image level %d < maskRes %d, lenX %d m_lenX>>image_level %d, lenY %d m_lenY>>image_level %d\n",image_level, m_motionMaskResolution, lenX, m_lenX>>image_level, lenY, m_lenY>>image_level);
      return false;
   }
   int nLevelsUp = m_motionMaskResolution - image_level;
   int upFactor = 1 << nLevelsUp;
   bool *mask_block_ptr = m_motionMaskReverse;
   bool *mask_ptr = mask_block_ptr;
   // the is imageBlock pointer is needed to store the start point of each pixel block inspected by the kernel
   // since each pixel block is inspected twice when kernel is empty
   unsigned char *imageBlock = image;
   // iterator ints
   int i, j, i2, j2, iIndentL, iIndentR, jIndentT, jIndentB;

   // dilate - should go back over this and expand for efficiency. no need to query j each time for boundary checking.
   //       do first lines. then break up each line to separate boundaries from non within a loop. then do last lines
   if (dilate) {
      int i3,j3;
      // for each pixel in motion mask array
      for (j = 0; j < masklenY; j++) {
         jIndentT = (j >= (int)dilateKernelHalfWidth ? -dilateKernelHalfWidth : 0);
         jIndentB = (j < masklenY - (int)dilateKernelHalfWidth ? dilateKernelHalfWidth : 0);
         for (i = 0; i < masklenX; i++) {
            // get kernel start and end points in row
            iIndentL = (i >= (int)dilateKernelHalfWidth ? -dilateKernelHalfWidth : 0);
            iIndentR = (i < masklenX - (int)dilateKernelHalfWidth ? dilateKernelHalfWidth : 0);
            // set kernel row start position
            mask_ptr = mask_block_ptr + jIndentT*masklenX + iIndentL;
            for (j2 = jIndentT; j2 <= jIndentB; j2++) {
               for (i2 = iIndentL; i2 <= iIndentR; i2++) {
                  if (*mask_ptr++) {
                     // set image pixel pointer to the block position
                     image = imageBlock;
                     for (j3 = 0; j3 < upFactor; j3++) {
                        for (i3 = 0; i3 < upFactor; i3++) {
                           *image = 255; // true
                           image += incX;
                        }
                        image += incY - upFactor*incX;
                     }
                     goto done;
                  }
               }
               mask_ptr += masklenX+iIndentL-iIndentR-1;
            }
            // don't set to false, leave as is
            /*
            // only reached if kernel is empty
            image = imageBlock;
            for (j3 = 0; j3 < upFactor; j3++) {
               for (i3 = 0; i3 < upFactor; i3++) {
                  *image = 0; //false
                  image += incX;
               }
               image += incY - upFactor*incX;
            }*/
            // jump to here if kernel has non-zero element
            done: imageBlock += upFactor*incX;
            mask_block_ptr++;
         }
         imageBlock += upFactor * (incY - incX * masklenX);
      }
   } else { // just up sample
      // mask pixel value holder
      unsigned char pix;
      // for each pixel in motion mask array
      for (j = 0; j < masklenY; j++) {
         for (i = 0; i < masklenX; i++) {
            /*// if you want to set the false pixels as well
            pix = *mask++ ? 255:0;
            for (j2 = 0; j2 < upFactor; j2++) {
               for (i2 = 0; i2 < upFactor; i2++) {
                  *image = pix;
                  image += incX;
               }
               image += incY - upFactor*incX;
            }*/
            if(*mask_ptr++) {
               for (j2 = 0; j2 < upFactor; j2++) {
                  for (i2 = 0; i2 < upFactor; i2++) {
                     *image = 255;
                     image += incX; // increment x by 1
                  }
                  image += incY - upFactor*incX; // move to begining of next row within block
               }
               // move to start of next block in the row (upFactor pixels along the x axis from start of last block)
               image += upFactor*(incX - incY);
            } else {
               // move to start of next block in the row
               image += upFactor*incX;
            }
         }
         // move to start position for next row of blocks
         // moved forward upFactor*incX*masklenX pixels in the row
         image += upFactor * (incY - incX * masklenX);
      }
   }
   return true;
    }  // if ot a camera motion mask
  else return false;  // don't write one of if ther isn't a valid one
}

bool PatchCorrespondenceGen::upsampleAndDilateNonCameraMotionMask(unsigned char *image, int incX, int incY, int lenX, int lenY, int image_level,
                          bool dilate, unsigned int dilateKernelHalfWidth) {
  if (m_gotValidCamMotionMask)  // if we've generated one....
    {
        int masklenX = m_lenX >> m_motionMaskResolution;
   int masklenY = m_lenY >> m_motionMaskResolution;
   //printf("m_len %d,%d masklen %d,%d\n",m_lenX,m_lenY,masklenX,masklenY);
   // dims of upsampled image
   if(image_level > m_motionMaskResolution || lenX != m_lenX >> image_level || lenY != m_lenY >> image_level || (dilate && dilateKernelHalfWidth<1)) {
      fprintf(stdout,"image level %d < maskRes %d, lenX %d m_lenX>>image_level %d, lenY %d m_lenY>>image_level %d\n",image_level, m_motionMaskResolution, lenX, m_lenX>>image_level, lenY, m_lenY>>image_level);
      return false;
   }
   int nLevelsUp = m_motionMaskResolution - image_level;
   int upFactor = 1 << nLevelsUp;
   bool *mask_block_ptr = m_motionMask;
   bool *mask_ptr = mask_block_ptr;
   // the is imageBlock pointer is needed to store the start point of each pixel block inspected by the kernel
   // since each pixel block is inspected twice when kernel is empty
   unsigned char *imageBlock = image;
   // iterator ints
   int i, j, i2, j2, iIndentL, iIndentR, jIndentT, jIndentB;

   // dilate - should go back over this and expand for efficiency. no need to query j each time for boundary checking.
   //       do first lines. then break up each line to separate boundaries from non within a loop. then do last lines
   if (dilate) {
      int i3,j3;
      // for each pixel in motion mask array
      for (j = 0; j < masklenY; j++) {
         jIndentT = (j >= (int)dilateKernelHalfWidth ? -dilateKernelHalfWidth : 0);
         jIndentB = (j < masklenY - (int)dilateKernelHalfWidth ? dilateKernelHalfWidth : 0);
         for (i = 0; i < masklenX; i++) {
            // get kernel start and end points in row
            iIndentL = (i >= (int)dilateKernelHalfWidth ? -dilateKernelHalfWidth : 0);
            iIndentR = (i < masklenX - (int)dilateKernelHalfWidth ? dilateKernelHalfWidth : 0);
            // set kernel row start position
            mask_ptr = mask_block_ptr + jIndentT*masklenX + iIndentL;
            for (j2 = jIndentT; j2 <= jIndentB; j2++) {
               for (i2 = iIndentL; i2 <= iIndentR; i2++) {
                  if (*mask_ptr++) {
                     // set image pixel pointer to the block position
                     image = imageBlock;
                     for (j3 = 0; j3 < upFactor; j3++) {
                        for (i3 = 0; i3 < upFactor; i3++) {
                           *image = 255; // true
                           image += incX;
                        }
                        image += incY - upFactor*incX;
                     }
                     goto done;
                  }
               }
               mask_ptr += masklenX+iIndentL-iIndentR-1;
            }
            // don't set to false, leave as is
            /*
            // only reached if kernel is empty
            image = imageBlock;
            for (j3 = 0; j3 < upFactor; j3++) {
               for (i3 = 0; i3 < upFactor; i3++) {
                  *image = 0; //false
                  image += incX;
               }
               image += incY - upFactor*incX;
            }*/
            // jump to here if kernel has non-zero element
            done: imageBlock += upFactor*incX;
            mask_block_ptr++;
         }
         imageBlock += upFactor * (incY - incX * masklenX);
      }
   } else { // just up sample
      // mask pixel value holder
      unsigned char pix;
      // for each pixel in motion mask array
      for (j = 0; j < masklenY; j++) {
         for (i = 0; i < masklenX; i++) {
            /*// if you want to set the false pixels as well
            pix = *mask++ ? 255:0;
            for (j2 = 0; j2 < upFactor; j2++) {
               for (i2 = 0; i2 < upFactor; i2++) {
                  *image = pix;
                  image += incX;
               }
               image += incY - upFactor*incX;
            }*/
            if(*mask_ptr++) {
               for (j2 = 0; j2 < upFactor; j2++) {
                  for (i2 = 0; i2 < upFactor; i2++) {
                     *image = 255;
                     image += incX; // increment x by 1
                  }
                  image += incY - upFactor*incX; // move to begining of next row within block
               }
               // move to start of next block in the row (upFactor pixels along the x axis from start of last block)
               image += upFactor*(incX - incY);
            } else {
               // move to start of next block in the row
               image += upFactor*incX;
            }
         }
         // move to start position for next row of blocks
         // moved forward upFactor*incX*masklenX pixels in the row
         image += upFactor * (incY - incX * masklenX);
      }
   }
   return true;
    }  // if ot a camera motion mask
  else return false;  // don't write one of if ther isn't a valid one
}


void PatchCorrespondenceGen::drawNonCameraMotionMask(unsigned char *image, int incX, int incY,
      int lenX, int lenY, bool dilate, unsigned int dilateKernelHalfWidth) {
   if(!upsampleAndDilateNonCameraMotionMask(image, incX, incY, lenX, lenY, 0, dilate, dilateKernelHalfWidth))
      fprintf(stdout,"PatchCorrespondenceGen::drawNonCameraMotionMask There was a problem drawing the motion mask. Dilate %d size %d\n",dilate,dilateKernelHalfWidth);
}

void PatchCorrespondenceGen::drawNonCameraMotionMaskReverse(unsigned char *image, int incX, int incY,
      int lenX, int lenY, bool dilate, unsigned int dilateKernelHalfWidth) {
   if(!upsampleAndDilateNonCameraMotionMaskReverse(image, incX, incY, lenX, lenY, 0, dilate, dilateKernelHalfWidth))
      fprintf(stdout,"PatchCorrespondenceGen::drawNonCameraMotionMask There was a problem drawing the motion mask. Dilate %d size %d\n",dilate,dilateKernelHalfWidth);
}

bool PatchCorrespondenceGen::videoStill() {
  int level = m_nResolutions-1;
  int scale = 1 << level;
  int lenX = m_lenX/scale;
  int lenY = m_lenY/scale;
  int incX = m_incX[level];
  int incY = m_incY[level];
  unsigned char *current  = m_image[m_cur][level];
  unsigned char *previous = m_image[m_prevButOne][level];
  int sum = 0;
  for ( int y = 0; y<lenY; y++ ) {
    for ( int x = 0; x<lenX; x++ ) {
      int index = ( y * incY ) + ( x * incX );
      sum = sum + abs( current[index] - previous[index] );
    }
  }
  if ( sum > 0 ) {
    return false;
  } else {
    return true;
  }
}
