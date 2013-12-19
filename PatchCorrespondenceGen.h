/*
 * File: PatchCorrespondenceGen.h
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


#ifndef PatchCorrespondenceGen_h
#define PatchCorrespondenceGen_h

#include "TexturePatch.h"  // class holding (multiple) representations of a (multi-resolution) texture patch at a given 3D location
#include "PoseEstPanTiltHead.h"
#include "KLTTracker.h"
#include "FittedLine.h"

#include "RandomForest.h"
#include "Tree.h"
#include "ForestObjects.h"

namespace bbcvp {

  /**
   *
   * Class to generate point correspondencies between an image and a set of stored (multi-resolution) patches.
   *
   * Includes methods to select new patches.
   **/

  class PatchCorrespondenceGen {

  public:

    /**
     * Constructor.  Allocates internal storage for working space.
     *
     * @param poseEst    Pointer to the PoseEstPanTiltHead object that holds the camera to use
     *
     * @param patchLenX, patchLenY  The dimensions of the patches to store, in pixels/lines (e.g. 16,16)
     *
     * @param len_x, len_y  The image dimensions (probably of a field, rather than a frame!).  It may be wise to
     *                      specify a portion of the image that ommits possible horizontal balnking (and maybe
     *                      vertical, as the first/last half-line may be black).  The image dimensions should be
     *                      divisible by 2^(nResolutions-1) in order to form a proper image pyramid.
     *
     * @param spacingX,spacingY The dimensions of the notional rectangles in the image in which we want to
     *                          have one feature - used to determine the overall feature density when adding new
     *                          features, but declared at the start as the feature-creation part relies on data
     *                          gathered during feature matching, so these spacings mustn't be changed once we've started.
     *
     * @param nResolutions Number of levels in the multi-resolution image pyramid used.  1=just use the full-res image.
     *
     * @param maxPatches The max number of texture patches to store across the whole image
     *
     * @param int maxPatchesPerRegion The max number of texture patches to store in one region
     *
     * @param maxFixedWorldlines The maxnumber of lines to project into the image and ignore features near
     * 
     * @param motionMaskResolution The image level of the motion mask, which indicates areas of non-camera real world motion to
     * 								be ignored.
     *
     * NB if you are working wiht multiple camera positions, you will
     * probably want a different set of patches for each (grabbed from
     * the viewpoint of the camera), and this should be handled by
     * having multiple instances of PatchCorrespondenceGen(), one for
     * each position.  The camera position index used with the
     * PoseEstPanTiltHead object should match the camera position for
     * the PatchCorrespondenceGen being used, as the
     * PoseEstPanTiltHead object holds a separate set of 3D patch
     * positions for each camera position, and the positions used must
     * correspond to the set of patches held internally by the
     * PatchCorrespondenceGen.  This allows multiple "cameras"
     * (i.e. grabbed images from a single camera position) to be used
     * to refine the camera position (as in the case of using
     * line-based estimation): the patch positions must be stored in
     * the PoseEstPanTiltHead object rather than being re-entered every time
     * findCorrespondencies() is called, in order to keep the 2D
     * observations in other "cameras" (ie. frames grabbed from this
     * camera) corresponding to the appropriate 3D points.
     *
     **/

    PatchCorrespondenceGen(PoseEstPanTiltHead *PoseEst,
            int patchLenX, int patchLenY,
            int len_x, int len_y,
            int spacingX, int spacingY, int nResolutions,
            int maxPatches, int maxPatchesPerRegion,
            int maxFixedWorldlines=0,
            int forestSize=0, int maxdepth=8, int threshold=10, int degree=3, int motionMaskResolution=-1);

    /**
     * Destructor.
     */
    ~PatchCorrespondenceGen();

    /**
     * Creates a monochrome multi-resolution "image pyramid" for use
     * in subsequent matching and patch-generation processes, form the
     * given rgb image.  The image previously held is retained
     * internally for image-to-image matching purposes.  Call this
     * once per image to process.
     *
     * @param r,g,b pointers to unsigned chars holding the incoming
     * RGB image
     *
     * @param inc_x,inc_y The spacing in memory between successive samples horizontally and vertically
     *
     * @param weightR, weightG, weightB  Weights (on scale 0..255) to apply when forming a single-component
     *        image from the given RGB image.  May work faster if one is 255 and others are zero.
     *
     *
     **/
    void loadImage(unsigned char *r, unsigned char *g, unsigned char *b,
         int inc_x, int inc_y,
         int weightR=76, int weightG= 150, int weightB=29);

    /**
     * Get a pointer to the image arrays for the current or previous image. Useful mainly for
     * testing the image pyramid generation.
     *
     * @param image 0 for current (the image most recently loaded), 1 for previous
     *
     * @return Pointer to array of pointers to the top-left pixels of the images in the pyramid.  Each image
     *         is an array of lenX x lenY pixels, where these lengths are the sizes of the image at that
     *         level.
     **/
    unsigned char **getImagePointers(int image=0);

    /**
     * Get pointer to array of x increments for most-recently-loaded image (assumed to be the same for
     * revious image as well)
     *
     * @return Pointer to array of x increments, length=number of levels in pyrmid
     **/
    int *getIncX() { return m_incX; }

    /**
     * Get pointer to array of y increments for most-recently-loaded image (assumed to be the same for
     * revious image as well)
     *
     * @return Pointer to array of x increments, length=number of levels in pyrmid
     **/
    int *getIncY() { return m_incY; }

    /**
     * Set subsampling factors used when searching for good features
     * to track in createNewPatches().
     *
     * @param featureFindSsX,featureFindSsY subsampling factors (1=no subsampling)
     *
     **/
    void setFeatureFindSubsample(int featureFindSsX, int featureFindSsY) { m_featureFindSsX=featureFindSsX; m_featureFindSsY=featureFindSsY; }

    /** Set minimum value of feature detector output that constitutes
     * a feature worth tracking.  Used in createNewPatches().
     *
     * @param minFeatureVal Minimum value (0=allow all)
     *
     **/
    void setMinFeatureVal(int minFeatureVal) { m_minFeatureVal=minFeatureVal; }

    /** Set minimum value of feature detector big eigenvalue output that constitutes
     * a feature worth tracking.  Used in createNewPatches().
     *
     * @param minFeatureValBigEigen Minimum value (0=allow all)
     *
     **/
    void setMinFeatureValBigEigen(int minFeatureValBigEigen) { m_minFeatureValBigEigen=minFeatureValBigEigen; }

    /**
     * Set the size of the region used for evaluating the 'strength'
     * of a feature when createNewPatches() looks for new
     * features. Note this is the "half-width", e.g. values of 5x3 mean
     * an area of 11x7 is evaluated.
     *
     * @param haldWidX,halfWidY The Half-widths of the window
     **/
    void setFeatureDetectionWindowHalfWid(int halfWidX, int halfWidY) { m_halfWidX=halfWidX; m_halfWidY=halfWidY; }

    /**
     * Set border around edge of image used when looking for features in
     * createNewPatches().  Should set to at least half patchLenX/Y (the
     * default is patchLenX & Y), or more, so that stored patches from at
     * least level 0 get filled with image texture (if they are not, the patch
     * size effectively shrinks asymmetrically).  Note that createNewPatches()
     * will look outside the border region, due to the sizes of the feature
     * finding filter & the local-max operation, but will adjust the area it
     * looks in so that no features will be found closer than these distances to the edge of the image.
     *
     * @param borderX,broderY The number of pixels around the image to ignore when finding features
     **/
    void setFeatureDetectionImageBorder(int borderX, int borderY) { m_featureCreateBorderX = borderX;  m_featureCreateBorderY = borderY; }

    /**
     * Set rectangular region of image to ignore when searching for features in createNewPatches().  Useful to make it only look
     * in areas that are likely to contain new features, and to make it ignore objects near the image centre that may be being
     * followed by a panning camera.
     *
     * @param minIgnoreX,maxIgnoreX,minIgnoreY,maxIgnoreY The range of the pixel rectangel to ignore
     *
     **/
    void setFeatureDetectionIgnoreRegion(int minIgnoreX, int maxIgnoreX, int minIgnoreY, int maxIgnoreY)
      { m_minIgnoreX=minIgnoreX; m_maxIgnoreX=maxIgnoreX; m_minIgnoreY=minIgnoreY; m_maxIgnoreY=maxIgnoreY; }

    /**
     * Set the min distance between features to find in createNewPatches().
     *
     * @param minDist distance in pixels below which the less-good feature(s) will be omitted
     *
     */
    void setMinDist(int minDist) {  m_minDist = minDist; }

    /**
     * Set the distance from projected line features from which point
     * features should be ignored.  Useful to stop features being
     * created on the jagged bits of lines on a running track.
     *
     * @param ignoreDistFromLine Distance, in pixels, from the line
     *
     **/
    void setIgnoreFeatureLines(double ignoreDistFromLine=0.0);

    /**
     * 
     * Set the (optional) pointers to arrays specifying triangle strips that
     * specify 'ignore' regions in the world, when calling createNewPatches
     *
     * @param nTriStrips Number of triangle strips.  If this is zero, there are no triangle strips and the pointers to the associated arrays can be zero.
     *
     * @param nTriPoints Number of points in each triangle strip.  Must be at least 3.
     *
     * @param triPoints Pointer to array of pointers to the triangle strip
     * arrays. If n is the triangle strip number 0..nTriStrips-1, and i is the
     * point in the triangle strip 0..nTriPoints[n], then
     * triPoints[n][3i+0..2] are the x,y,z vertices of the triangle.
     *
     * @param ignoreWithinTriangles Set true to ignore regions of the image that project to inside a triangle, or false to ignore all points that do NOT project within a triange.
     *
     **/
    void setIgnoreTriStrips(int nTriStrips, int *nTriPoints=(int*)0, double **triPoints=(double**)0, bool ignoreWithinTriangles=true);

    /**
     * 
     * Whether or not to use the 'ignore' regions specified by setIgnoreTriStrips
     * Useful if you want to define an ignore region for use when creating permanent patches but later to allow temporary patches with the region
     *
     * @param useIgnoreTriStrips Set true to enable use of ignore regions or false to disable
     *
     **/
    void setUseIgnoreTriStrips( bool useIgnoreTriStrips=true) { m_useIgnoreTriStrips = useIgnoreTriStrips; }

    /**
     * Specify whether the motion mask computed using
     * detectNonCameraMotion() should be used as an ignore region by
     * createNewPatches() and findCorrespondences()
     *
     **/
    void setUseMotionMaskAsIgnoreRegion(bool useMotionMaskAsIgnoreRegion=true) { m_useMotionMaskAsIgnoreRegion = useMotionMaskAsIgnoreRegion; }

    /**
     * Specify whether motion mask is computed across a frame period
     * or a field period.  Generally it is better to compute the
     * motion mask across a frame period, as this reduces false
     * positives from interlace on vertical detail with a static
     * camera, and also amplifies the effect of foreground motion,
     * making it easier to detect.  The disadvantage is that revealed
     * background areas (which also get flagged as motion) will be
     * larger.  The default (set tin the constructor) is computation
     * across a frame.
     *
     * @param measureAcrossFramePeriod Set true to compute motion mask
     * across a frame period (two image periods) or false to compute
     * between adjacent image.
     **/
    void setMotionMaskComputationAcrossFrame( bool measureAcrossFramePeriod=true) { m_measureMotionMaskAcrossFramePeriod=measureAcrossFramePeriod; }

    /**
     * Specify the maximum fraction of visible texture patches to
     * ignore using the motion mask.  A value of 1 means that all
     * patches falling inside the motion mask will be discarded (both
     * for learning and tracking); a value of 0.5 means that no more
     * than 50% will be discarded.  Generally it is sensible not to
     * discard more than around 50%, to prevent complete failure of
     * the tracker if there is a temporary glitch in the tracking or
     * something else (like a flash in the image) that might
     * temporarily make most or all parts of the image not match well.
     *
     * @param maxFractionToDiscardUsingMotionMask Max fraction of features to discard
     **/
    void setMaxFractionToDiscardUsingMotionMask(double fraction) { m_maxFractionToDiscardUsingMotionMask = fraction; }

    /**
     * Set the level of the image pyramid used for feature finding by findNewFeatures().  Defaults to 0 if not set.
     *
     * @param level The elvel to use (0=fullres, max value = nResolutions-1)
     *
     **/
    void setFeatureFindLevel(int level=0) { m_featureFindLevel = level; }

    /**
     * Create a new texture patch from the current image at the given 2D and 3D coordintes.
     * Used internally by AddPatchesFromImageUsingEstimatedPose() when automatically
     * creating patches, but may also be called during initial
     * calibration to create a patch at a user-defined point in the image having user-defined
     * world coordinates.  The focal length of the camera associated with the created patches comes
     * from the current value in the associated PoseEstPanTiltHead object (as will any other
     * info needed, like the roll of the camera, or the normal vector of the patch, if this is ever implemented!)
     *
     * @param imageX,imageY  Coords in image of pixel at centre of patch to extract
     *
     * @param worldX,worldY,worldZ  Coords of point in world
     *
     * @param fixedAxis Whether the patch has a user-specified fixed absolute position - if set to zero,
     *        the positon of the patch should not be refined during scene calibration, if set to 1,2 or 3
     *        then the position of the patch should only be constrained in its x, y, or z coord respectively.
     *
     * @param expiryTime Determines whether this patch is deleted when
     * deleteOldPatches() is called.  A value of -1 means "live forever".
     *
     * @param patchVelX,patchValY image velocity in pixels/lines per image that patch was taken from.
     *
     * @return true if patch created successfully, false if max number of patches reached
     **/
    bool createNewPatch(int imageX, int imageY, double worldX, double worldY, double worldZ, int fixedAxis=0,
         int expiryTime=-1, double patchVelX=0.0, double patchVelY=0.0);

    /**
     * Add a new texture representation to the given patch (e.g. at a different focal length, or
     * potentially from a different view direction).
     * Parameters for focal length, etc. are taken from current camera state.
     *
     * @param patchID Index number of patch to update
     *
     * @param imageX,imageY  Coords in image of pixel at centre of patch to extract
     **/
    bool addNewTextureToPatch(int patchID, double imageX, double imageY);

    /**
     * Create new patch(es) at "good" int incX, int incY, locations.  Image is notionally divided into regions as specified in
     * the constructor, and for all regions that did not have feature projected into them, a new feature
     * is created at the location having the best "feature signature".  The 3D location of the feature is
     * determined using the current camera pose, projecting the feature out to the plane (xy, xz or yz)
     * that is closest to being at right angles to the current camera view direction.
     *
     * @param minPatchesPerRegion The minimum number of patches per
     * region we want to have.  Regions having fewer than this number
     * of patches will be searched to try to create new features.
     *
     * @param nRegionsToSearch Number of regions to create features
     * in.  Leave as default (-1) to allow it to create features in
     * all regions in the image, but a smaller value can be specified
     * to limit the amount of time that the patch searching takes.
     * Searching starts at the block where it left off last time.  The
     * specified limit applies to the number of regions that are
     * actually searched, not the number that are checked to see if
     * they contain too few features.
     *
     * @param expiryTime Determines whether this patch is deleted when
     * deleteOldPatches() is called.  A value of -1 means "live
     * forever".  NB when creating patches that live forever, the only
     * patches checked for closeness are other permanent patches,
     * although when creating temporary patches, the closeness to both
     * temp and permanent patches is considered.
     *
     * @return number of new features created
     *
     **/
    int createNewPatches(int minPatchesPerRegion=1, int nRegionsToSearch=-1, int expiryTime=-1);

    /**
     * Delete all stored patches
     **/
    void deleteAllPatches();

    /**
     * Delete all stored patches whose expiry time is less than or equal to the given value.
     *
     * @param expiryTime the value to which each patch expiry time
     * will be compared: those less than or equal will be deleted.
     *
     * @return the number of patches deleted
     *
     **/
    int deleteOldPatches(int expiryTime);

    /**
     * Delete all patches that are not visible in any images currently
     * stored in the PoseEstPanTiltHead class (this includes patches
     * viewed by the current camera (ie. in the current image), and
     * ones previously stored.  Useful if you want to compute a global
     * optimisation of camera parameters and patch positions across
     * all stored images: it is impossible to refine the 3D positions
     * of patches that are not visible in at least ome image, and if
     * the patches were left in their current positions they would
     * probably end up being positioned inconsistently with other
     * patches around them.
     *
     * @return number of patches deleted
     **/
    int deleteUnseenPatches();

    /**
     * Update the reliability measure of all patches based on the outcome of
     * the latest RANSAC call.  This updates the counts of the number of
     * times each patch was observed, and the number of times it was deemed
     * to be an inlier.  These values are used by the deleteUnreliablePatches() method.
     *
     **/
    void updatePatchReliabilityMeasure();

    /**
     * Update the focal length value associatyed with each stored
     * patch, folloing a call to
     * PoseEstPanTiltHead::calcFullSolution().  The full solution
     * calculation refines patch positions and will end up with camera
     * parameters (including focal length) for the stored reference
     * images that are different (and hopefully more correct!) than
     * those assumed when the patcher were first captured.  This means
     * that the assumed focal lengths for the stored paches will be
     * inconsistent with their new positions.  For each patch, this
     * method looks at the ratio of the change of focal length in the
     * first stored reference image that the patch was visible in, and
     * scales the patch focal length by this amount.  This should mean
     * that the patches will undergo the same scaling when the tracker
     * is operating on this reference after refinement as it had
     * before refinement.
     **/
    void updatePatchFocalLength();

    /**
     * Reset the reliability measure of all patches back to the initial state, i.e. as if the patches had never been visible.
     *
     **/
    void resetPatchReliabilityMeasure();

    /**
     * Delete patches that have only been inliers for less than a given proportion of the times they have been visible.
     *
     * @param minFractionVisible Proportion of the visible times that it has been an inlier below which the patch should be deleted
     *
     * @param minTimesVisibleForSensibleMeasurement Only apply the test once patches have been seen for at least this number of times
     *
     * @return the number of pathces deleted
     *
     **/
    int deleteUnreliablePatches(double minFractionVisible=0.5, int minTimesVisibleForSensibleMeasurement=4);

    /**
     * Delete the patch with the given index number.
     *
     * @param patchIndex index of patch to delete, in the range 0..nPatches-1
     *
     * @return true if deleted OK, false if patch number was out of range
     **/
    bool deletePatch(int patchIndex);

    /**
     * Draw the position of the currently-stored patches on the given
     * image, by projecting them into the image using the current camera pose.
     *
     * @param image top-left pixel of image to draw onto
     *
     * @param incX,incY increments between pixels in image
     *
     * @param lenX,lenY size of image
     *
     **/
    void drawProjectedPatches(unsigned char *image, int incX, int incY, int lenX, int lenY);

    /**
     * Draw the position of the currently-stored permanent patches
     * (with expiryTime <0) on the given image, by projecting them
     * into the image using the current camera pose.  Each patch is
     * drawn as a square around it.
     *
     * @param image top-left pixel of image to draw onto
     *
     * @param incX,incY increments between pixels in image
     *
     * @param lenX,lenY size of image
     *
     **/
    void drawPermanentPatches(unsigned char *image, int incX, int incY, int lenX, int lenY);

    void drawPermanentPatchesClassification(unsigned char *image, unsigned char *imageAlt, int incX, int incY, int lenX, int lenY);

    /**
     * Generate correspondences between stored patches and the current image (as set with SetImage()),
     * using the current camera pose as an intial estimate.  Correspondences found are added to the
     * PoseEstPanTilt object specified in the constructor.  No pose computation or outlier rejection
     * takes place at this stage - do that with the PoseEstPanTilt object.
     *
     * @param matchErrToHalveWeight average error per pixel that will
     * cause weight to be halved, so poor correspondencies have a
     * reduced effect: weight =
     * matchErrToHalveWeight/(matchErrToHalveWeight+matchErr)
     *
     * @param usePrevIm Whether to perform an initial estimation using the image pyramid PREVIOUSLY
     *                  specified - i.e. field-to-field tracking
     *
     * @param useStoredPatches Whether to refine the patch positions using the stored image patches, to
     *                         eliminate drift. This step will be done by default using:
     *                   -DC normalisation,
     *                   -using only the top level of the image pyramid
     *                   -with scale correction for the relative focal length of the camera as currently set in
     *                    PoseEstPanTiltHead compared to the focal length of the camera when the patch was captured
     *
     * @param stopIfWeightDrops If true, stop at a level in the multi-res image pyramid if the next level
     *                    would give a higher uncertainty
     *
     * @param normaliseDC Whether to subtract the mean DC level from the patches before computing KLT
     *
     * @param highestLevelStoredPatches The highest resolution level ( lowest resolution image )
     * to start with when using the stored patches.  Set to -1 to use the highest level available
     *
     * @param lowestLevelStoredPatches The lowest resolution level ( highest resolution image )
     * to descend to when using the stored patches.  Set to 0 for full resolution image
     *
     * @param lowestLevelPrevPatches The lowest resolution level ( highest resolution image )
     * to descend to when using the previous image patches.  Set to 0 for full resolution image
     *
     * @param maxDiscardFraction Discard up to this fraction of correspondences from centre of image ( 0 = don't discard any )
     *
     * @param discardRadius Discard any correspondences found within this number of pixels from image centre,
     * up to discardFraction of total points found
     *
     * @param minScaleChangeToComputeScaledPatches The minimum change
     * in the ratio of the current focal length to that when a patch
     * was stored to make it worth rescaling the patch.  Eg. set to
     * 99999 to that patches are never resacled.
     *
     * @param maxWeight Clip feature weights to this value ( 0=don't clip )
     *
     * @param useWeights Whether or not to use feature weights. If false, all features will be set to a constant weight.
     *
     * @param weightScale Scaling to apply to weight of all points. Useful to change weighting given to points if both
     * points and lines are being used, so that their relative weighting can be adjusted.
     *
     * @return Number of correspondences generated
     **/

    int findCorrespondences(double matchErrToHalveWeight,
             bool usePrevIm = true,
             bool useStoredPatches = true,
             bool stopIfWeightDrops = false,
             bool normaliseDC = true,
             int highestLevelStoredPatches=-1,
             int lowestLevelStoredPatches=0,
             int lowestLevelPrevPatchesincX=0,
                            double maxDiscardFraction=0.0,
                            double discardRadius=0.0,
             double minScaleChangeToComputeScaledPatches=99999.0,
                            double maxWeight=0.0,
                            bool   useWeights=false,
             double weightScale = 1.0);

    /**
      * Generate correspondences between stored patches and the current image (as set with SetImage()),
      * using the current camera pose as an intial estimate.  Correspondences found are added to the
      * PoseEstPanTilt object specified in the constructor.  No pose computation or outlier rejection
      * takes place at this stage - do that with the PoseEstPanTilt object.
      *
      * Unlike findCorrespondences(), which estimates the translation between
      * predicted and observed patch positions separately for each patch, this
      * method performs this golbally, for all patches that were deemed to be
      * inliers in the last calcSolutionRANSAC call (or uses all if last
      * solution calc was non-RANSAC).  It computes the translation, scale
      * change and rotation that minimises the luminance mismatch (after
      * optional DC normalisation), then applies these to each patch position
      * stored in the PoseEstPanTiltHead object.
      *
      * @param useStoredPatches true=from stored patches, false=from previous image
      * @param normaliseDC whether to normalise DC
      * @param highestLevel most subsampled level to use  (-1=use hightest available)
      * @param lowestLevel least subsampled level to use (0=full-res)
      * @param stopIterShift Stop iterating when shift (in pixels in full-res image) is below this
      * @param stopIterScaleChange Stop iterating when scale change is less than this
      * @param stopIterCount Stop iterating once this number of iterations have been carried out,
      *     even if shift and scale change are still changing a lot
      *
      * @return number of patches visible, or, if failed to be able to compute
      * a solution (e.g. too few patches visible, return a negatve fail code as produced by the Solve class.
      */

    int calcCorrespondenciesGlobally(
   bool useStoredPatches,  // true=from stored patches, false=from previous image
   bool normaliseDC,  // whether to normalise DC
   int highestLevel,  // most subsampled level to use  (-1=use hightest available)
   int lowestLevel,   // least subsampled level to use (0=full-res)
   double stopIterShift=0.1, double stopIterScaleChange=0.001, int stopIterCount=10);  // controls for iteration stopping

    /**
     * Perform an exhaustive search over the given range of pan, tilt, roll, and vertical field-of-view, trying to find
     * pre-stored features, and using RANSAC to compute a solution.
     *
     * @param pMin,pMax,pStepPixels Min and Max pan angles in degrees, and
     * number of pixels to pan by between steps
     *
     * @param tMin,tMax,tStepPixels Tilt range (in degrees) and step size in terms of pixels
     *
     * @param rMin,rMax,rNStep Roll range (in degrees) and number of steps
     *
     * @param fMin,fMax,fStepPixels Vertical field-of-view range (in degrees) and step size in terms of pixels
     *
     * @param maxInlierDist The max. distance in pixels that points and/or
     * line ends can be from the reporjected positions to be considered inliers
     *
     * @param minInlierFraction The minimum fraction of total points+lines that
     * must be inliers for the process to stop.
     *
     * @param maxTries Maximum number of random sets to try before giving up
     *
     * @param nInitialPoints The number of points to randomly select
     *
     * @param lenYFull The number of vertical lines corresponding to
     * the field-of-view angles given (will usually be the number of
     * lines in a frame, without any borders being subtracted)
     *
     */
    double exhaustiveSearch(double pMin, double pMax, double pStepPixels,
             double tMin, double tMax, double tStepPixels,
             double rMin, double rMax, int rNStep,
             double fMin, double fMax, double fStepPixels,
             double maxInlierDist=6.0, double minInlierFraction=0.7, int maxTries=10,
             int nInitialPoints=5, int lenYFull=576);

    /**
     * Use classifier method to match features then
     * use RANSAC to compute a solution.
     *
     *
     * @param maxInlierDist The max. distance in pixels that points and/or
     * line ends can be from the reporjected positions to be considered inliers
     *
     * @param minInlierFraction The minimum fraction of total points+lines that
     * must be inliers for the process to stop.
     *
     * @param maxTries Maximum number of random sets to try before giving up
     *
     * @param nInitialPoints The number of points to randomly select
     *
     */
    double classifierInitialisation(double maxInlierDist=6.0, double minInlierFraction=0.7, int maxTries=10,
          int nInitialPoints=5, int maxNewFeaturesToFind = 250, int cycle = -1, int subsample = 0, double initialiseScale = 1.0);


    /**
     * Return a pointer to the array of texture patches.  Mainly for
     * testing purposes, e.g. to write out or display the stored
     * patches.
     *
     * @param nPatches Write the number of texture patches currently stored here.
     *
     * @return Pointer to an array of pointers to texture patch objects.
     **/
    TexturePatch **getTexturePatches(int *nPatches) { *nPatches =  m_nPatches; return (m_texturePatches); }

    /**
     * Return the number of texture patches currently stored
     *
     **/
    inline int getNPatches() { return (m_nPatches); }

    /**
     * Set the minimum change in position below which iterations in
     * the KLT are stopped in the singleLevelTrack() method.  This is rather like setting the spatial
     * accuracy that you want the KLT to try to achieve.  When a
     * position update with a length below that specified here is
     * applied, the iteration process stops.
     *
     * @param KLTAccuracy displacement value change in pixels below which to stop iterating
     **/
    void setKLTAccuracy(double KLTAccuracy) { m_KLTTracker->setKLTAccuracy(KLTAccuracy); }

    /**
     * For the createNewPatches() method, set the fraction of the
     * difference between big & small eigenvalues to subtract from the
     * small eigenvalue when creating the 'feature value' to use for
     * ranking features.  The traditional method is to find points
     * where the smallest eigenvalue (related to the average gradient
     * in the direction of minimum gradient) is biggest.  However,
     * this can result in features being found on near-horizontal
     * lines, where vertical aliasing due to interlace creates
     * 'jagged' lines.  Such lines should have a very strong gradient
     * in the other direction (i.e. the biggest eigenvector), so
     * bysubtracting a portion of the difference between the two, such
     * features should be downweighted.
     *
     * Setting this value to 0 (as set by default in the constructor) leaves the feature finder using just the smallest eigenvelue.
     *
     * Setting this to 1.0 would result in downweighting to zero any features where the biggest eigenvalue was at least twice the smallest.
     *
     * @param costOfEigenvalueDiff The weighting factor to use
     **/
    void setCostOfEigenvalueDiff(double costOfEigenvalueDiff) { m_KLTTracker->setCostOfEigenvalueDiff(costOfEigenvalueDiff); }

    /**
     * Set the maximum number of iterations in a single resolution level that the KLT tracker should carry out.
     *
     * @param maxIter Max number of iterations
     **/
    void setmaxIter(int maxIter) {m_KLTTracker->setmaxIter(maxIter); }

    /**
     * Write texture patch data to the  given file.
     * Writes the data  relating to the currently-stored patches to the given file, in a binary
     * format (not human-readable, but readable by readPatches() ).
     *
     * If you only want to store permanent patches, then call deleteOldPatches(<big number>) first, where
     * <big numbere> is at least the current frame count plus the patch lifetime.
     *
     * @param filename Name of file to write data to
     **/
    bool writePatches(char *fileName);

    /**
     * Read texture patch data from the  given file.
     * Over-writes any current patch data.  Data file must be in format produced by writePatches().
     *
     * @param filename Name of file to read data from
     **/
    bool readPatches(char *fileName);

    /**
     * Draw the position of all the patches that are found to be possible features.  Each patch is
     * drawn as a line.
     *
     * @param image top-left pixel of image to draw onto
     *
     * @param incX,incY increments between pixels in image
     *
     * @param lenX,lenY size of image
     *
     **/
    void drawSuggestedFeatures(unsigned char *image, int incX, int incY, int lenX, int lenY);

    /**
     * Clear the arrays containing the suggested features. This will also mean that the following frame the screen clears of suggested features.
     *
     **/
    void wipeSuggestedFeatures()
    {
      if (printMatchX.size() > 0)
      {
         printMatchX.resize(0);
         printMatchY.resize(0);
         printMatchScore.resize(0);
       }
    };

    /**
     * Draw line across the top of the screen that marks the current level of
     * correspondence between tracker and classifier
     *
     * The line appears pink
     *
     * @param image top-left pixel of image to draw onto
     *
     * @param incX,incY increments between pixels in image
     *
     * @param lenX,lenY size of image
     *
     **/
    void drawCheckCorrespondenceLine(unsigned char *image, int incX, int incY, int lenX, int lenY);

    /**
     * Add to the classifier all features from the current scene. The added features must be perminant
     * (ie, have a persistant ID). They must also pass certain tests within the function, e.g that the
     * camera has moved since they were last added or that the classifier is currently failing to find them.
     *
     **/
    void addPermanentPatchesClassifier();

    /**
     * Generate correspondences between stored patches and the current image by classifying those patches
     * in the current image using the random forest classifier.
     *
     * @param mode optional argument that can be used to change the workings of the function.
     *
     * @return Number of correspondences generated
     **/
    int findClassifierCorrespondences(int mode = 0, int maxNewFeaturesToFind = 250, int cycle = -1, int subsample = 0, double initialiseScale = 1.0);

    /**
     * Compare the current set of perminant patches in the scene with those in the classifier to see if they match correctly.
     * This function is designed to be run regularly to allow a test to discover if the tracking has lost it's bearing or
     * entered an unknown space.
     *
     * @return A score representing the succesful number of matches.
     **/
    double checkCorrespondences(bool allowFreeMove, double initialiseScale = 1.0, bool ignoreOutliers = false);

    /**
     * Perform an exhaustive search using the classifier method. Though it returns a result, it doesn't
     *  change the tracker position.
     *
     *
     * @param maxInlierDist The max. distance in pixels that points and/or
     * line ends can be from the reporjected positions to be considered inliers
     *
     * @param minInlierFraction The minimum fraction of total points+lines that
     * must be inliers for the process to stop.
     *
     * @param maxTries Maximum number of random sets to try before giving up
     *
     * @param nInitialPoints The number of points to randomly select
     *
     */
    double testClassifierInitialisation(double maxInlierDist=6.0, double minInlierFraction=0.7, int maxTries=10,
          int nInitialPoints=5, int maxNewFeaturesToFind = 250, int cycle = -1, int subsample = 0, double initialiseScale = 1.0);

    /**
     * Set velocity range around a stored patch's velocity within which the
     * patch will be taken into account when looking to learn new patches.
     * Set to a large number to turn this feature off and make patch learning
     * independent of the velocity at which other stored patches were learned.
     *
     * Values used by createNewPatches().
     *
     * @param velRangeX,velRangeY The velocity ranges, in pixels and lines between successive images
     *
     **/
    void setVelocityRangeForNoRelearning(double velRangeX, double velRangeY) { m_VelocityRangeForNoRelearningX = velRangeX;
    m_VelocityRangeForNoRelearningY = velRangeY; }
 
   /**
     * Set the ratio between the stored patch's focal length and the current focal length below which the
     * patch will be taken into account when looking to learn new patches.
     * 
     * Set to a large number to turn this feature off and make patch learning
     * independent of the focal length at which other stored patches were learned.
     *
     * Values used by createNewPatches().
     *
     * @param focalLengthRatio The ratio above which the patch will not inhibit the learning of new patches in the surrounding region
     *
     **/
    void setFocalLengthRatioForNoRelearning(double focalLengthRatio ) { m_FocLenRatioForNoRelearning = focalLengthRatio; }

    /**
     * Set the max number of patches we should search per region This allows
     * the number of patches being searched to be limited to less than are
     * actually visible.  Discarding of surplus patches is prioritised by how
     * the camera motion speed and focal length matches those when the patches
     * were captured.
     *
     * @param maxPatchesToSearchPerRegion the max number of patches to search per region
     **/
    void setMaxPatchesToSearchPerRegion(int maxPatchesToSearchPerRegion) {m_maxPatchesToSearchPerRegion = maxPatchesToSearchPerRegion; }

    /**
     * Set the parameters used to control how patches are dropped when more
     * than maxPatchesToSearchPerRegion are visible in a region
     *
     **/
     void setPatchPrefWeights(double patchPrefVelWeight, double patchPrefFoclenWeight) { m_patchPrefVelWeight=patchPrefVelWeight; m_patchPrefFoclenWeight=patchPrefFoclenWeight; }

     /**
      * Look through all stored patches and ignore those that should be ignored according to the cuurently-defined
      * triangle strip ignore region (or 'include' region, according to the defined polarity.
      *
      * @return numbr of patches deleted
      **/
     int discardStoredPatchesUsingTriStripRegion();

    bool writeClassifier(char *fileName);
    bool readClassifier(char *fileName);
    void optimiseClassifier();

	/**
	 * Gets coarse projection of current image into previous image using lineOfSight method (use PoseEstimator method to get line of sight vector in camera reference system)
	 * and produces a difference mask for the current image area. The resulting binary image can be used as an indicator of non-camera real-world motion, as opposed to observed
	 * image motion as a result of camera movement.
	 *
	 * @param threshold The threshold for pixel difference between
	 * the two successive frames that defines non-camera motion.
	 *
	 * @param nProjX,nProjY The number of sample points across the
	 * image that the projection between images is computed at
	 * (with bilinear interpolation being used to fill in the
	 * others). Each should be at least 2, and an odd number
	 * should be used so as to have a sample oint at the centre of
	 * the image to better interpolate in the presence of zoom.
	 * If lens distortion is to be accurately accounted for, then
	 * a higher numbre should be used.
	 * 
	 * @return The function will return false if the motion mask array will not have been initialised (i.e. motion mask resolution was not supplied in constructor)
	 * or if there are less than two images stored in the pose estimator.  
	 */
	bool detectNonCameraMotion(unsigned char threshold, int nProjX=5, int nProjY=5);

	bool detectNonCameraMotionReverse(unsigned char threshold, int nProjX=5, int nProjY=5);

	/**
	 * Indicates whether motion at this image point can be trusted, based on non-camera motion between the two previous frames.
	 * 
	 * @note This function is available to allow diagnostic output of the generated motion mask (stored at m_motionMask).
	 *
	 * @param x,y The co-ordinates in the image (with borders ignored i.e. 0,0 is the top-left pixel that is within the borders)
	 */
	bool getNonCameraMotion(int x, int y);

	/**
	 * Return a pointer to the camera motion mask, to allow an
	 * external method to quickly query it rather than going
	 * through the getNonCameraMotion() method.  Remember that the
	 * mask is subsampled by a factor of motionMaskResolution in
	 * each direction, with x any y lengths rounded down.
	 *
	 * @return address of array of bools (one per subsampled pixel), or NULL if motion mask is not un use
	 **/
	bool *getNonCameraMotionMask() { return m_motionMask; }

	bool *getNonCameraMotionMaskReverse() { return m_motionMaskReverse; }

    /**
     * Draw the non-camera motion mask on the given image using the stored mask array.
     *
     * @param imageR, imageG, imageB top-left pixel of image to draw onto for each colour channel
     *
     * @param incX,incY increments between pixels in image
     *
     * @param lenX,lenY size of image
     * 
     * @param dilate If true, then the motion mask is dilated (morphological operation) as it is drawn.
     * 
     * @param dilateKernelHalfWidth Determines the size of the dilation kernel if used. A value of 2 results in a 5x5 kernel, 1 gives a 3x3 kernel etc.
     *
     **/
    void drawNonCameraMotionMask(unsigned char *image,
            int incX, int incY, int lenX, int lenY, bool dilate=false, unsigned int dilateKernelHalfWidth=1);

    void drawNonCameraMotionMaskReverse(unsigned char *image,
            int incX, int incY, int lenX, int lenY, bool dilate=false, unsigned int dilateKernelHalfWidth=1);

    /**
     * Specify that a motion mask is not valid, e.g. if the camera position has been changed, to prevent it from being used
     * for controlling enabling/disabling of features and from being drawn on output
     **/
    void invalidateCamMotionMask() { m_gotValidCamMotionMask = false; m_firstGoodImage=m_nImagesLoaded;}

    /**
     * Compare the current frame to the previous frame, by subtracting the lowest resolution images.
     * Can be used to decide whether the image velocity and motion mask are useful or not.
     * 
     * @return true if the two frames are identical, false if they are different. 
     * 
     **/
    bool videoStill();

  private:
    bool pointInTriangle( double Px, double Py, double *A, double *B, double *C );
    PoseEstPanTiltHead *m_PoseEst;       // PoseEstPanTiltHead object to use
    TexturePatch **m_texturePatches;     // array of pointers to stored texture patches
    KLTTracker *m_KLTTracker;
    int m_nPatches;                      // number of patches stored so far
    int m_patchLenX, m_patchLenY;        // dimensions of patches (ie blocks)
    int m_spacingX, m_spacingY;          // typical spacing between created features
    int m_nResolutions;                  // number of resolution levels for multi-resolution matiching
    unsigned char *m_r, *m_g, *m_b;      // top-left pixel in current image
    int m_maxPatches;                    // max number of patches to store
    int m_lenX, m_lenY;                  // dims of image
    int m_weightR, m_weightG, m_weightB; // coeffs to make single comp image from
    unsigned char ***m_image;            // image[cur/prev][level][pixel offset]
    int *m_incX, *m_incY;                // x & y increments for each level of image pyramid - computed once
    int m_cur, m_prev, m_prevButOne;     // indices for cur & prev image (=1,0 or 0,1)
    int m_nImagesLoaded;                 // number of times loadImage() called
    unsigned char *m_tempImage;          // for holding image while subsampling
    int **m_nPatchesSeen;                // 2D array that counts number of patches seen in each region
    int ***m_patchPosX;                  // 3D array that holds the patches seen  in each region
    int ***m_patchPosY;                  // ditto - y coord
    int ***m_patchIndex;                 // ditto - patch index
    bool ***m_notePatchWhenLearning;     // ditto - wheter patch should be considered when learning new patches
    double *m_usefulness;                // ditto - usefulness of patches in region being processed
    int m_nRegionsX, m_nRegionsY;        // number of regions that image is divided into for patch creation
    int m_maxPatchesPerRegion;           // the max number of patches per region we can hold
    int m_arrayLenForFeatureFind;        // length of arrays to pass to findGoodFeatures - needs to be longer than maxPatchesPerRegion
    int m_maxPatchesPerRegionForDistanceCheck;  // storage to allocate for checking nearness of prev points to potential new ones
    int m_featureFindSsX, m_featureFindSsY; // step size when searching for maxima
    int m_minFeatureVal;                 // min magnitude of feature detector output to consider
    int m_minFeatureValBigEigen;         // min magnitude of feature detector big eigenvalue output to consider
    int m_halfWidX, m_halfWidY;          // half-widths of feature detection window used when finding good features
    int m_minDist;                       // min distance between features to find
    int m_featureCreateBorderX;          // ignore this number of pixels around image edge in findNewFeatures()
    int m_featureCreateBorderY;          // ignore this number of pixels around image edge in findNewFeatures()
    int m_minIgnoreX, m_maxIgnoreX;      // ignore this rectangle of pixels within image.....
    int m_minIgnoreY, m_maxIgnoreY;      // ...when looking for features in findNewFeatures
    int m_maxPatchesToSearchPerRegion;   // max number of patches that we should do a KLT search for
    double m_patchPrefVelWeight;         // amount to reduce patch pref by for each pix-per-field-period velocity difference
    double m_patchPrefFoclenWeight;      // amount to reduce patch pref by for each factor of difference between focal lengths
    int m_nextRegionToSearch;            // next patch to search in createNewPatches()
    int *m_regionToSearchX;              // array of patch coordintes, in the order they are to be searched
    int *m_regionToSearchY;              // ditto for Y coords
    int *m_FeatureX, *m_FeatureY;        // to hold coords of features found in createNewPatches()
    int *m_FeatureVal;                   // to hold strength of features found
    double *m_imX, *m_imY;               // used in findCorrespondencies, allocated in constructor
    double **m_directionalWeight;        // ditto
    double *m_kltResult;                 // ditto
    bool *m_patchVisible;                // ditto
    double *m_prevPosX, *m_prevPosY;     // ditto
    double m_ignoreDistFromLineSquared;  // min distance squared for the above check
    FittedLine *m_linesToIgnore;         // array of pointers to lines for above check
    int m_maxFixedWorldLines;            // number of lines to allocate storage for
    double m_VelocityRangeForNoRelearningX;  // min abs X diff between current im vel & stored patch im vel before patch won't stop new ones being learned
    double m_VelocityRangeForNoRelearningY;  // ditto for Y
    double m_FocLenRatioForNoRelearning;  // ratio of patch focal length to current focal length below which patch will stop new ones being learned
    int m_featureFindLevel;              // level of image pyramid to use for feature finding
    bool *m_motionMask; 				 // store a mask indicating pixels to ignore due to real world motion between the two previous frames
    bool *m_motionMaskReverse; 				 // store a mask indicating pixels to ignore due to real world motion between the two previous frames
    double *m_interFrameMappingX;		 // store the pixel mappings from previous frame to current frame at the motion mask resolution
    double *m_interFrameMappingY;
    int m_motionMaskResolution;			 // the image resolution of the motion mask stored as depth in the image pyramid i.e full res * 2^(-n)
    bool m_useMotionMaskAsIgnoreRegion;  // whether motion mask prevents learning and tracking of points
    bool m_gotValidCamMotionMask;        // whether we have computed a valid camera motion mask
    int m_firstGoodImage;                // the number of images loaded when invalidateCamMotionMask was called
    double m_maxFractionToDiscardUsingMotionMask;  // limit of what fraction of points to discard using motion massk in findCorrespondednces()
    bool m_measureMotionMaskAcrossFramePeriod;     // whether to compute motion mask across two images (i.e. across a frame) (true) or across one image (false)
    int m_nTriStrips;                    // number of triangle strips defining an ignore region
    int *m_nTriPoints;                   // m_nTriPoints[n] - the number of points in the nth triangle strip
    double **m_triPoints;                // m_triPoints[n][3i+0..2] - the x/y/z coords of the ith point in the nth triangle strip
    bool m_ignoreWithinTriangles;        // true if ignoring regions within triangles, or false to only use regions in triangles
    double **m_projTri;                  // m_projTri[n][2i+0..1] storage space for the projected vertices of each visible triangle, in pixels (allocated in setIgnoreTriStrips)
    bool **m_projTriPointUsable;         // m_triPointUsable[n][i] - whether the ith point in the nth tristrip is in front of the camera (allocated in setIgnoreTriStrips)
    bool m_useIgnoreTriStrips;           // whether or not to use the triStrip ignore regions

// Things for initialisation:

    int *m_FeatureTempX, *m_FeatureTempY;        // to hold coords of features found in createNewPatches()
    int *m_FeatureTempVal;                   // to hold strength of features found

    randomForest *rf;                    //The classifer
    imageDetails *id;                    //Object through which the classifier access the image
    typedef std::pair<int, int > int_int_pair; //Types of pair used in evaluating the best feature matches
    typedef std::vector<int_int_pair> vec_pair;

    struct coord_prob {
      int x;
      int y;
      int index;
      double prob;
   };


    typedef std::vector<coord_prob> coord_prob_v;

    coord_prob_v match_array;

   int partition(const int top, const int bottom);
   void quicksort(int top, int bottom);

   
   
   bool upsampleAndDilateNonCameraMotionMask(unsigned char *image, int incX, int incY, int lenX, int lenY, int image_level, 
		   										bool dilate=false, unsigned int dilateKernelHalfWidth=1);

   bool upsampleAndDilateNonCameraMotionMaskReverse(unsigned char *image, int incX, int incY, int lenX, int lenY, int image_level, 
		   										bool dilate=false, unsigned int dilateKernelHalfWidth=1);


    int match_array_index;
    std::map<int, int> indexlistMap;
    std::map<int, double> scorelistMap;


    bool freeMove;                       // Flag used in the feeMove work, still under construction.


  //TEMPORARY
    std::vector<int>printMatchX;
    std::vector<int>printMatchY;
    std::vector<int>printMatchScore;
    double printMatch;

    int addToClassifier(int, int, int);
    void buildClassifier();


  };  // end of class BlockCorrespondenceGen

}  // end of bbcvp namespace

#endif
