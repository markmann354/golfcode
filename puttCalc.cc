
//
//	This is a example how to use the command parser
//	O. Grau, Nov-2001

#include        <vplibs/VPutil/PoseEstimator.h>
#include	<stdlib.h>
#include <stdio.h>
#include	<string.h>
#include	<vplibs/VPbase/Picture.h>
//#include	<Picture.h>
#include	"puttCalc.h"
//#include	<VPError.h>
//#include	<PicReadFileSeq.h>
//#include	<WriteImage.h>
#include	<vplibs/VPbase/VPError.h>
#include	<vplibs/VPcoreio/PicReadFileSeq.h>
#include	<vplibs/VPcoreio/WriteImage.h>

#include        <vplibs/VPip/Region.h>
//#include        <Region.h>
#include        "objectDetect.cc"
#include        "functions.cc"
//#include        "xml_writer.h"
//#include        "Picture.h"
#include <iomanip>

#include <sstream>
#include <iostream>
#include <fstream>

#include <string>
#include <vector>

#include        <vplibs/VPutil/ptr_to_ogl.h>
#include        <vplibs/VPip/InitialPoseEstLines.h>               // for the method for finding pixels on lines (not used for initialisation despite its name!)
#include        <vplibs/VPip/PatchCorrespondenceGen.h>            // NB includes PoseEstimator.h which #define's X, Y Z (bad practice!) which messes up thigs in PitchModel so must be after it
#include        "posProject.h"
#include        "posProject.cc"
#include        "segmenter.cpp"
#include        "initialisation.cpp"

#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <vplibs/VPutil/UnixTerminal.h>

#define NUM_THREADS 2;

using std::ifstream;

//#include	"Deinterlace.h"
#define PI = 3.14159625

// Marck

using namespace std;


bool writeOutCam(char *camFileName, int frame, double pos_x, double pos_y, double pos_z,
                 double pan, double tilt, double roll,
                 double focalLength, int widthPixels, int heightPixels, double notionalPixHeight, double notionalPixWidth)
{
    FILE *fp;


    if((fp = fopen(camFileName, "w")) == NULL)
    {
        printf("Cannot create file %s\n", camFileName);
        return 0;
    }


    double view_x, view_y, view_z, up_x, up_y, up_z;
    bbcvp::ptr_to_viewvec_upvec_deg(pan, tilt, roll, &view_x, &view_y, &view_z, &up_x, &up_y, &up_z);



    //  double centreX, centreY;
    //  myPoseEst->getCamPixelsSize(&centreX, &centreY);

    notionalPixWidth =  (((notionalPixHeight*576)/9)*16)/720; // notionalPixHeight;//


    //  fprintf(fp, "%s\n", camFileName.c_str());

    fprintf(fp, "#SOGEM V1.1 ascii\n");
    fprintf(fp, "Camera{\n");
    fprintf(fp, "\tIdent\t%d\n", int(frame));
    fprintf(fp, "\tCVec\t%f\t%f\t%f\n", pos_x, pos_y, pos_z);
    //fprintf(fp, "\tCVec\t%f\t%f\t%f\n", 0.0, 0.0, 0.0);
    fprintf(fp, "\tAVec\t%f\t%f\t%f\n", view_x, view_y, view_z);
    fprintf(fp, "\tUpVec\t%f\t%f\t%f\n", up_x, up_y, up_z);
    fprintf(fp, "\tWidth\t%d\n", widthPixels);
    fprintf(fp, "\tHeight\t%d\n", heightPixels);
    fprintf(fp, "\tPixelSizeX\t%g\n", notionalPixWidth);
    fprintf(fp, "\tPixelSizeY\t%g\n", notionalPixHeight);
    fprintf(fp, "\tCenterPointShiftX\t%d\n", int(0));
    fprintf(fp, "\tCenterPointShiftY\t%d\n", int(0));
    fprintf(fp, "\tFocalLength\t%f\n", focalLength);
    fprintf(fp, "\tIris\t%d\n", int(1));
    fprintf(fp, "\tReciprocalFocus\t%d\n", int(0));
    fprintf(fp, "\tExposureTime\t%d\n", int(0));
    fprintf(fp, "}\n");

    fclose(fp);
    return 0;
}

template<typename T>
std::string to_string(T n)
{
    std::ostringstream os;
    os << n;
    return os.str();
}

typedef struct _myst
{
    int a;
    char b[10];
}myst;

void cbfunc(myst *mt)
{
    fprintf(stdout,"called %d %s.",mt->a,mt->b);
}



/*
int getch(void)
{
  struct termios oldt, newt;
  int ch;
  tcgetattr( STDIN_FILENO, &oldt );
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr( STDIN_FILENO, TCSANOW, &newt );
  ch = getchar();
  tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
  return ch;
}
*/

class Button
{
public:
    //
    // Fields
    //
    int X;
    int Y;
    int Width;
    int Height;
    // Our callback
    void (*OnCreateCallback)(void);

    //
    // Functions
    //
    Button(int x, int y, int w, int h, void (*callback)(void));
    void OnCreate();
};

char getch() {
        char buf = 0;
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0)
                perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0)
                perror("tcsetattr ICANON");
        if (read(0, &buf, 1) < 0)
                perror ("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0)
                perror ("tcsetattr ~ICANON");
        return (buf);
}



Button::Button(int x, int y, int w, int h, void (*callback)(void))
{
    X = x;
    Y = y;
    Width = w;
    Height = h;
    OnCreateCallback = callback;

    // Trigger the callback
    OnCreate();

}

void Button::OnCreate()
{
    // Make sure the callback isn't null
    if(OnCreateCallback != 0)
    {
        OnCreateCallback();
    }
}

void Button1Created()
{
    printf("button1 created\n");
}





void drawpoint(bbcvp::CPicture<unsigned char> *output, int x, int y, int colour, int number, int screenWidth, int screenHeight){

    for (int r = -3; r < 4; r++){
        for (int s = -3; s < 4; s++){
            if(     x < screenWidth -4 && x > 3 &&
                    y < screenHeight -4 && y > 3 &&
                    (r == s || r == -s || r == 1-s || r == 1+s)
                    ){
                output->SetPixel(colour,x+r,y+s,0);
                output->SetPixel(colour,x+r,y+s,1);
                output->SetPixel(colour,x+r,y+s,2);
            }
        }
    }

    if(     x < screenWidth -20 && x > 20 &&
            y < screenHeight -20 && y > 20
            ){

        if (number < 10){
            drawnumber(output,x+10,y+10,0,number);
        }
        else{
            int firstdigit = number/10;
            int seconddigit = number-firstdigit*10;
            drawnumber(output,x+10,y+10,0,firstdigit);
            drawnumber(output,x+15,y+10,0,seconddigit);
        }

    }

}

//void CALLBACK getContinue (){

//}

int function(){
    int ch = getch();
    cout << "INPUT " << ch << endl;
    return ch;
}

int DLLFunc(int(*callbackfunc)()){
    // Check if recieves Video then
    callbackfunc();
}

int
ltestarg( _puttCalc_parameter &para)
{
    pthread_t threads[2];

    bbcvp::UnixTerminal term;



    int min_flag = 0;
    vpcoreio::PicReadFileSeq	picseq;
    vpcoreio::PicReadFileSeq	picseq2;
    vpcoreio::PicReadFileSeq	picseq3;
    vpcoreio::PicReadFileSeq	picseq4;
    bbcvp::s64		frame_count;
    bbcvp::ErrCode	err;
    bbcvp::ErrCode	err2;
    bbcvp::ErrCode	err3;
    bbcvp::ErrCode	err4;
    char	buf[1000];
    bool has3Dgeometry = false; //true;
    //const char	buf2[41] =  "originalclips/jimenezapproach/green.tiff";

    // pass the arguments through
    if( para.fn_flag)
        sprintf(buf,"-fn %s -fn2 %s -start %0d -end %0d -inc %d -fmt %d -dx %d -dy %d -filt %d",
                para.fn,para.fn2,para.start,para.end, para.inc,
                para.fmt, para.dx, para.dy ,para.filt ); //, para.deint);
    else
        sprintf(buf,"-patt %s -patt2 %s -start %0d -end %0d -inc %d -fmt %d -dx %d -dy %d -filt %d",
                para.patt,para.patt2,para.start,para.end, para.inc,
                para.fmt, para.dx, para.dy, para.filt );
    if(para.cam_flag) sprintf(buf+strlen(buf)," -cam %s",para.cam);
    else if(para.campatt_flag) sprintf(buf+strlen(buf)," -campatt %s",para.campatt);

    // print out current argument list
    std::cout<<"Arg: "<<buf<<"\n";

    std::cout<<"para.patt: "<<para.patt<<"\n";



    //
    // Open image sequence
    //-patt /home/markma/Videos/idiotfilming/CLIPS001/XF0111/pictures/%04d.png -patt2 /home/markma/Videos/idiotfilming/CLIPS001/XF0006/pictures/%04d.png -start "1000" -end "1200"
    char buf1[1000];
    char buf2[1000];
    char buffer [4];
    char buffer2 [4];
    char buffer3 [4];
    char buffer4 [4];

    int imgDiff = 68;

    bool input = false;



    std::string str = to_string(para.start);
    strcpy(buffer, str.c_str());
    std::string str2 = to_string(para.end);
    strcpy(buffer2, str2.c_str());

    std::string str3 = to_string(para.start - imgDiff);
    strcpy(buffer3, str3.c_str());
    std::string str4 = to_string(para.end - imgDiff);
    strcpy(buffer4, str4.c_str());

    cout << "buffer " << buffer << endl;
    cout << "buffer2 " << buffer2 << endl;
    cout << "buffer3 " << buffer3 << endl;
    cout << "buffer4 " << buffer4 << endl;


    if( para.fn_flag){

        strcpy (buf1," -patt ");
        strcat (buf1, para.fn);
        strcat (buf1, " -start ");
        strcat (buf1, buffer);
        strcat (buf1, " -end ");
        strcat (buf1, buffer2);

        strcpy (buf2," -patt ");
        strcat (buf2, para.fn2);
        strcat (buf2, " -start ");
        strcat (buf2, buffer3);
        strcat (buf2, " -end ");
        strcat (buf2, buffer4);
    }

    else{
        strcpy (buf1," -patt ");
        strcat (buf1, para.patt);
        strcat (buf1, " -start ");
        strcat (buf1, buffer);
        strcat (buf1, " -end ");
        strcat (buf1, buffer2);

        strcpy (buf2," -patt ");
        strcat (buf2, para.patt2);
        strcat (buf2, " -start ");
        strcat (buf2, buffer3);
        strcat (buf2, " -end ");
        strcat (buf2, buffer4);
    }



    err  = picseq.Open( buf1 );
    err2 = picseq2.Open( buf2 );
    err3 = picseq3.Open( "-patt /home/markma/Videos/idiotfilming/cam1green.tiff -start 1 -end 1" );
    err4 = picseq4.Open( "-patt /home/markma/Videos/idiotfilming/cam2green.tiff -start 1 -end 1");

    //cout << "*************************************" << endl;
    char *bufft = para.patt;
    int idxToDel = strlen(bufft)-8;
    memmove( &bufft[ idxToDel ] , &bufft[ idxToDel+8 ], strlen( bufft ) - idxToDel ) ;
    char *buffy = para.patt2;
    idxToDel = strlen(buffy)-8;
    memmove( &buffy[ idxToDel ] , &buffy[ idxToDel+8 ], strlen( buffy ) - idxToDel ) ;
    //cout << "*************************************" << endl;


    const char *tmp;
    tmp = err==bbcvp::OK?"OK!":"Error!";
    std::cout<<"Err: "<<tmp<<"; code="<<err<<std::endl;
    if (err!=bbcvp::OK) { return err; }

    const char *tmp2;
    tmp2 = err2==bbcvp::OK?"OK!":"Error!";
    std::cout<<"Err2: "<<tmp2<<"; code="<<err2<<std::endl;
    if (err2!=bbcvp::OK) { return err2; }

    const char *tmp3;
    tmp3 = err3==bbcvp::OK?"OK!":"Error!";
    std::cout<<"Err3: "<<tmp3<<"; code="<<err3<<std::endl;
    if (err3!=bbcvp::OK) { return err3; }

    const char *tmp4;
    tmp4 = err4==bbcvp::OK?"OK!":"Error!";
    std::cout<<"Err4: "<<tmp4<<"; code="<<err4<<std::endl;
    if (err4!=bbcvp::OK) { return err4; }

    std::cout << "printfBuf: " << buf << std::endl;
    std::cout << "printfErr: " << err << std::endl;

    std::cout << "printfBuf2: " << buf2 << std::endl;
    std::cout << "printfErr2: " << err2 << std::endl;

    //	Loop over all selected frames of sequence
    //
    //bbcvp::TimeCode	lastcam_tc;
    //lastcam_tc = picseq.GetCamera().GetTimeStamp();

    const int seq_len  = picseq.GetNumberOfPictures();
    const int seq_len2 = picseq2.GetNumberOfPictures();
    std::cout<<"Length two "<<seq_len2<<std::endl;
    int frames_to_process = seq_len / para.inc;
    if (seq_len % para.inc > 0)
        ++frames_to_process;
    std::cout<<"Processing every "<<para.inc<<". frame between ["<<
               para.start<<";"<<para.end<<"] ==> "<<

               frames_to_process<<" frames in total."<<std::endl;

    int frameNumber[frames_to_process];
    double ballDistance[frames_to_process];

    for(frame_count=0; frame_count<frames_to_process; frame_count++)
    {
        frameNumber[frame_count] = 0;
        ballDistance[frame_count] = 0;
    }

    int totalnumberofpixels = 0;
    int screenWidth = 0;
    int screenHeight = 0;

    bbcvp::PoseEstPanTiltHead *myPoseEst;




    // RUN A LOOP OF ONE TO DETERMINE THE SIZE OF THE PICTURE

    // TODO: add some parallelisation here?
    for( frame_count=0; frame_count<1; frame_count++ ) {
        err = picseq.LoadNextPicture();
        if(err!=bbcvp::OK) {
            // if end of sequence is reached, program doesn't return error!
            if (frame_count == 1) {
                std::cout<<"Initialization complete - let's spot some balls: "<<frame_count<<std::endl;
                err = bbcvp::OK;
            } else {
                std::cerr<<"Err: picseq.LoadNextPicture() returned: "<<
                           err<<", frame_count="<<frame_count<<
                           ", frames_to_process="<<1-frame_count<<std::endl;
            }
            break;
        }

        bbcvp::CPicture<unsigned char> picBuf = picseq.GetPicture();
        totalnumberofpixels = picBuf.GetNx()*picBuf.GetNy();
        screenWidth = picBuf.GetNx();
        screenHeight = picBuf.GetNy();

    }

    bbcvp::CPicture<unsigned char> regGrid1(screenWidth, screenHeight,1);
    bbcvp::CPicture<unsigned char> regGrid2(screenWidth, screenHeight,1);

    bbcvp::CPicture<unsigned char> cam1green(screenWidth, screenHeight,1);
    bbcvp::CPicture<unsigned char> cam2green(screenWidth, screenHeight,1);

    err3 = picseq3.LoadNextPicture();
    err4 = picseq4.LoadNextPicture();
    cam1green = picseq3.GetPicture();
    cam2green = picseq4.GetPicture();


    //*************CAMERA ONE*************************//

    //REGISTRATION MARKS AS THEY APPEAR ON THE SCREEN

    double RegOneX = 1680;
    double RegOneY = 232;
    double RegTwoX = 42;
    double RegTwoY = 262;


    //ARRAYS FOR STORING ALL 3D WORLD POSITIONS AS THEY APPEAR ON THE SCREEN WITH ASSOCIATED ERRORS

    double *coordinatearray_x = new double[totalnumberofpixels];
    double *coordinatearray_y = new double[totalnumberofpixels];
    double *coordinatearray_z = new double[totalnumberofpixels];
    double *error_x = new double[totalnumberofpixels];
    double *error_y = new double[totalnumberofpixels];
    double *error_z = new double[totalnumberofpixels];
    int *averageCounter = new int[totalnumberofpixels];


    //ARRAYS FOR STORING THE 3D WORLD POSITIONS OF DETECTED GOLF BALLS

    double *golfworld_x = new double[totalnumberofpixels];
    double *golfworld_y = new double[totalnumberofpixels];
    double *golfworld_z = new double[totalnumberofpixels];


    //ARRAYS FOR STORING THE 2D SCREEN POSITIONS OF THE DETECTED GOLF BALLS

    double *golfscreen_x = new double[totalnumberofpixels];
    double *golfscreen_y = new double[totalnumberofpixels];
    double *M1golfscreen_x = new double[totalnumberofpixels];
    double *M1golfscreen_y = new double[totalnumberofpixels];
    bool *isObject = new bool[totalnumberofpixels];
    bool *M1isObject = new bool[totalnumberofpixels];
    bool *M2isObject = new bool[totalnumberofpixels];
    bool *isObject2 = new bool[totalnumberofpixels];
    int *objectCounter = new int[totalnumberofpixels];

    for(int i=0;i<totalnumberofpixels;i++){
        coordinatearray_x[i]=0;
        coordinatearray_y[i]=0;
        coordinatearray_z[i]=0;
        error_x[i] = 0;
        error_y[i] = 0;
        error_z[i] = 0;
        averageCounter[i]=0;
        golfscreen_x[i]=0;
        golfscreen_y[i]=0;
        M1golfscreen_x[i]=0;
        M1golfscreen_y[i]=0;
        golfworld_x[i]=0;
        golfworld_y[i]=0;
        golfworld_z[i]=0;
        objectCounter[i]=0;
        isObject[i]=false;
        M1isObject[i]=false;
        M2isObject[i]=false;
        isObject2[i]=false;
    }


    bbcvp::CPicture<unsigned char> fieldmap(screenWidth, screenHeight,3);
    bbcvp::CPicture<unsigned char> fieldmapTwo(screenWidth, screenHeight,3);

    // position of registration marks in the real world - HARD CODED AT THE MOMENT

    long double RealWorldRegOneX = 0;//5696654.799;
    long double RealWorldRegOneY = 0;//69.507;
    long double RealWorldRegOneZ = 0;//666143.861;
    long double RealWorldRegTwoX = -1.33;//5696636.232;
    long double RealWorldRegTwoY = 0;//69.332;
    long double RealWorldRegTwoZ = 13.13;//666155.180;

    long double CamPosX = 12.75;//5696670.839;
    long double CamPosY = 1.4;//76.146;
    long double CamPosZ = 9.67;//666180.0;


    int maxCameras = 200;              // max number of images used when calibrating one camera (not the number of actual cameras)
    int maxPoints = 100;  // max number of texture patches to store
    int maxRefParallelLines = 5;      // max number of reference lines whose direction is known but whose absolute position is not
    int maxRefWorldPoints = 4;       // max number of fixed world points we'll use when specifying the calibration
    int maxRefWorldLines = 50;       // max number of lines - in this case, all will be on the ref object but PatchCorGen will allocate space on all obejcts
    int maxPointsPerLine = 2;

    // geometry of the scene

    //Nothing here yet to deal with interlace I assume?
    double aspect =  (16.0 * (double)screenWidth / 702.0) / (9.0 * (double)screenHeight / 576.0);

    myPoseEst = new bbcvp::PoseEstPanTiltHead(maxRefWorldLines,  // max number of lines on reference objects
                                              maxCameras,        // max number of images used to calibrate a camera position
                                              maxPoints+maxRefParallelLines+1,  // maxObjects = max patches we'll find, plus max parallel ref lines plus one for points in the fixed ref frame
                                              screenWidth, screenHeight*2, aspect,  // image dims and full aspect ratio
                                              maxPointsPerLine,        // allocate storage for this number of points on each line (usually 2 unless using all points)
                                              1,  // max positions - we only consider one camera location
                                              maxRefWorldPoints,      // max points on a reference object: determined by the max 'fixed' world points we'll have
                                              maxRefParallelLines+1);  // max number of reference objects (that can have lines on them) = 1 plus any lines with part-unknown coords

    initialisation(
                coordinatearray_x,
                coordinatearray_y,
                coordinatearray_z,
                error_x,
                error_y,
                error_z,
                &fieldmap,
                RegOneX,
                RegOneY,
                RegTwoX,
                RegTwoY,
                RealWorldRegOneX,
                RealWorldRegOneY,
                RealWorldRegOneZ,
                RealWorldRegTwoX,
                RealWorldRegTwoY,
                RealWorldRegTwoZ,
                totalnumberofpixels,
                CamPosX,
                CamPosY,
                CamPosZ,
                myPoseEst,
                has3Dgeometry
                );



    char buff[1000];

    if(has3Dgeometry){
        sprintf(buf, "golfgrid%04d.tiff", 1);
        vpcoreio::WriteImage(fieldmap, buf);
    }




    bbcvp::PoseEstPanTiltHead *TwomyPoseEst;

    long double TwoCamPosX = 0;//5696670.839;
    long double TwoCamPosY = 0.67;//76.146;
    long double TwoCamPosZ = -1.5;//666125.0;

    bool cameraTwo = true;




    //*************CAMERA TWO*************************//




    //REGISTRATION MARKS AS THEY APPEAR ON THE SCREEN

    double TwoRegOneX = 1193;//1000;
    double TwoRegOneY = 203;//400;
    double TwoRegTwoX = 1378;//1200;
    double TwoRegTwoY = 207;//160;


    //ARRAYS FOR STORING ALL 3D WORLD POSITIONS AS THEY APPEAR ON THE SCREEN WITH ASSOCIATED ERRORS

    double *Twocoordinatearray_x = new double[totalnumberofpixels];
    double *Twocoordinatearray_y = new double[totalnumberofpixels];
    double *Twocoordinatearray_z = new double[totalnumberofpixels];
    double *Twoerror_x = new double[totalnumberofpixels];
    double *Twoerror_y = new double[totalnumberofpixels];
    double *Twoerror_z = new double[totalnumberofpixels];
    int *TwoaverageCounter = new int[totalnumberofpixels];


    //ARRAYS FOR STORING THE 3D WORLD POSITIONS OF DETECTED GOLF BALLS

    double *Twogolfworld_x = new double[totalnumberofpixels];
    double *Twogolfworld_y = new double[totalnumberofpixels];
    double *Twogolfworld_z = new double[totalnumberofpixels];


    //ARRAYS FOR STORING THE 2D SCREEN POSITIONS OF THE DETECTED GOLF BALLS

    double *Twogolfscreen_x = new double[totalnumberofpixels];
    double *Twogolfscreen_y = new double[totalnumberofpixels];
    double *M2golfscreen_x = new double[totalnumberofpixels];
    double *M2golfscreen_y = new double[totalnumberofpixels];


    for(int i=0;i<totalnumberofpixels;i++){
        Twocoordinatearray_x[i]=0;
        Twocoordinatearray_y[i]=0;
        Twocoordinatearray_z[i]=0;
        Twoerror_x[i] = 0;
        Twoerror_y[i] = 0;
        Twoerror_z[i] = 0;
        TwoaverageCounter[i]=0;
        Twogolfscreen_x[i]=0;
        Twogolfscreen_y[i]=0;
        M2golfscreen_x[i]=0;
        M2golfscreen_y[i]=0;
        Twogolfworld_x[i]=0;
        Twogolfworld_y[i]=0;
        Twogolfworld_z[i]=0;
    }

    if(cameraTwo){


        // position of registration marks in the real world - HARD CODED AT THE MOMENT

        long double TwoRealWorldRegOneX = 0;//5696654.799;
        long double TwoRealWorldRegOneY = 0;//69.507;
        long double TwoRealWorldRegOneZ = 13.63;//666143.861;
        long double TwoRealWorldRegTwoX = RealWorldRegTwoX;//5696636.232;
        long double TwoRealWorldRegTwoY = RealWorldRegTwoY;//69.332;
        long double TwoRealWorldRegTwoZ = 13.63;//666155.180;






        int TwomaxCameras = 200;              // max number of images used when calibrating one camera (not the number of actual cameras)
        int TwomaxPoints = 100;  // max number of texture patches to store
        int TwomaxRefParallelLines = 5;      // max number of reference lines whose direction is known but whose absolute position is not
        int TwomaxRefWorldPoints = 4;       // max number of fixed world points we'll use when specifying the calibration
        int TwomaxRefWorldLines = 50;       // max number of lines - in this case, all will be on the ref object but PatchCorGen will allocate space on all obejcts
        int TwomaxPointsPerLine = 2;

        // geometry of the scene




        //Nothing here yet to deal with interlace I assume?
        double Twoaspect =  (16.0 * (double)screenWidth / 702.0) / (9.0 * (double)screenHeight / 576.0);



        TwomyPoseEst = new bbcvp::PoseEstPanTiltHead(TwomaxRefWorldLines,  // max number of lines on reference objects
                                                     TwomaxCameras,        // max number of images used to calibrate a camera position
                                                     TwomaxPoints+TwomaxRefParallelLines+1,  // maxObjects = max patches we'll find, plus max parallel ref lines plus one for points in the fixed ref frame
                                                     screenWidth, screenHeight*2, Twoaspect,  // image dims and full aspect ratio
                                                     TwomaxPointsPerLine,        // allocate storage for this number of points on each line (usually 2 unless using all points)
                                                     1,  // max positions - we only consider one camera location
                                                     TwomaxRefWorldPoints,      // max points on a reference object: determined by the max 'fixed' world points we'll have
                                                     TwomaxRefParallelLines+1);  // max number of reference objects (that can have lines on them) = 1 plus any lines with part-unknown coords


        initialisation(
                    Twocoordinatearray_x,
                    Twocoordinatearray_y,
                    Twocoordinatearray_z,
                    Twoerror_x,
                    Twoerror_y,
                    Twoerror_z,
                    &fieldmapTwo,
                    TwoRegOneX,
                    TwoRegOneY,
                    TwoRegTwoX,
                    TwoRegTwoY,
                    TwoRealWorldRegOneX,
                    TwoRealWorldRegOneY,
                    TwoRealWorldRegOneZ,
                    TwoRealWorldRegTwoX,
                    TwoRealWorldRegTwoY,
                    TwoRealWorldRegTwoZ,
                    totalnumberofpixels,
                    TwoCamPosX,
                    TwoCamPosY,
                    TwoCamPosZ,
                    TwomyPoseEst,
                    has3Dgeometry
                    );

        if(has3Dgeometry){
            sprintf(buf, "golfgrid%04d.tiff", 2);
            vpcoreio::WriteImage(fieldmapTwo, buf);
        }


    }



    double b2Wx1 = 0;
    double b2Wy1 = 0;
    double b2Wz1 = 0;
    double b2Wx2 = 0;
    double b2Wy2 = 0;
    double b2Wz2 = 0;
    double b2Wx3 = 0;
    double b2Wy3 = 0;
    double b2Wz3 = 0;
    double b2Wx4 = 0;
    double b2Wy4 = 0;
    double b2Wz4 = 0;
    double bP2x = 0;
    double bP2y = 0;
    double bP1x = 0;
    double bP1y = 0;


    TwomyPoseEst->lineOfSight(screenWidth/2,screenHeight/6, &b2Wx1, &b2Wy1, &b2Wz1);
    myPoseEst->lineOfSight(screenWidth/2,screenHeight/4, &b2Wx2, &b2Wy2, &b2Wz2);
    TwomyPoseEst->lineOfSight(screenWidth/2,screenHeight, &b2Wx3, &b2Wy3, &b2Wz3);
    myPoseEst->lineOfSight(screenWidth/2,screenHeight, &b2Wx4, &b2Wy4, &b2Wz4);
    //  ballWorldX = 5-CamPosX;
    //  ballWorldY = 0-CamPosY;
    //  ballWorldZ = 8-CamPosZ;
    //  ball2WorldX = 5-TwoCamPosX;
    //  ball2WorldY = 0-TwoCamPosY;
    //   ball2WorldZ = 8-TwoCamPosZ;

    double multiplier = 0;

    multiplier = -TwoCamPosY/b2Wy1;
    cout << "Multiplier " << multiplier << endl;
    double x2min1 = multiplier*b2Wx1+TwoCamPosX;
    double z2max1 = multiplier*b2Wz1+TwoCamPosZ;
    cout << "x:z " << x2min1 << "," << z2max1 << endl;

    multiplier = -CamPosY/b2Wy2;
    cout << "Multiplier " << multiplier << endl;
    double x2min2 = multiplier*b2Wx2+CamPosX;
    double z2max2 = multiplier*b2Wz2+CamPosZ;
    cout << "x:z " << x2min2 << "," << z2max2 << endl;

    multiplier = -TwoCamPosY/b2Wy3;
    cout << "Multiplier " << multiplier << endl;
    double x2min3 = multiplier*b2Wx3+TwoCamPosX;
    double z2max3 = multiplier*b2Wz3+TwoCamPosZ;
    cout << "x:z " << x2min3 << "," << z2max3 << endl;

    multiplier = -CamPosY/b2Wy4;
    cout << "Multiplier " << multiplier << endl;
    double x2min4 = multiplier*b2Wx4+CamPosX;
    double z2max4 = multiplier*b2Wz4+CamPosZ;
    cout << "x:z " << x2min4 << "," << z2max4 << endl;

    double xMin = 100000000;
    double zMin = 100000000;
    double xMax = -100000000;
    double zMax = -100000000;

    if(x2min1 < xMin){
        xMin = x2min1;
    }
    if(x2min2 < xMin){
        xMin = x2min2;
    }
    if(x2min3 < xMin){
        xMin = x2min3;
    }
    if(x2min4 < xMin){
        xMin = x2min4;
    }
    if(z2max1 < zMin){
        zMin = z2max1;
    }
    if(z2max2 < zMin){
        zMin = z2max2;
    }
    if(z2max3 < zMin){
        zMin = z2max3;
    }
    if(z2max4 < zMin){
        zMin = z2max4;
    }
    if(x2min1 > xMax){
        xMax = x2min1;
    }
    if(x2min2 > xMax){
        xMax = x2min2;
    }
    if(x2min3 > xMax){
        xMax = x2min3;
    }
    if(x2min4 > xMax){
        xMax = x2min4;
    }
    if(z2max1 > zMax){
        zMax = z2max1;
    }
    if(z2max2 > zMax){
        zMax = z2max2;
    }
    if(z2max3 > zMax){
        zMax = z2max3;
    }
    if(z2max4 > zMax){
        zMax = z2max4;
    }

    double bx[289];
    double by[289];
    double bz[289];
    double g1x[289];
    double g1y[289];
    double g2x[289];
    double g2y[289];

    for(int r = 0 ; r < 17 ; r++) {
        for(int j = 0; j < 17 ; j++){
            bx[17*r+j] = (xMax-xMin)/16*r+xMin;
            by[17*r+j] = 0;
            bz[17*r+j] = (zMax-zMin)/16*j+zMin;
        }
    }

    /*
double f = ballWorldX*ball2WorldX + ballWorldY*ball2WorldY + ballWorldZ*ball2WorldZ;

double t1 = (TwoCamPosX-CamPosX)*ballWorldX + (TwoCamPosY-CamPosY)*ballWorldY + (TwoCamPosZ-CamPosZ)*ballWorldZ;
double t2 = (TwoCamPosX-CamPosX)*ball2WorldX + (TwoCamPosY-CamPosY)*ball2WorldY + (TwoCamPosZ-CamPosZ)*ball2WorldZ;
double d1 = (t1 - f*t2)/(1-f*f);
double d2 = (t2 - f*t1)/(1-f*f);
long double p1x = CamPosX + d1*ballWorldX;
long double p1y = CamPosY + d1*ballWorldY;
long double p1z = CamPosZ + d1*ballWorldZ;

long double p2x = TwoCamPosX - d2*ball2WorldX;
long double p2y = TwoCamPosY - d2*ball2WorldY;
long double p2z = TwoCamPosZ - d2*ball2WorldZ;

cout << "Real World Position 1: X,Y,Z " << p1x << "," << p1y << "," << p1z << endl; //If the two vectors don't intersect
cout << "Real World Position 2: X,Y,Z " << p2x << "," << p2y << "," << p2z << endl; //These two won't be equal

*/

    for (int h = 0 ; h < 289; h ++){
        g1x[h] = 0;
        g1y[h] = 0;
        g2x[h] = 0;
        g2y[h] = 0;
        double bWX = bx[h];
        double bWY = by[h];
        double bWZ = bz[h];

        double bWX2 = bx[h];
        double bWY2 = by[h];
        double bWZ2 = bz[h];

        TwomyPoseEst->projectIntoImageArbitraryPoint(bWX2 - TwoCamPosX,bWY2 - TwoCamPosY,bWZ2 - TwoCamPosZ,&bP2x,&bP2y);
        myPoseEst->projectIntoImageArbitraryPoint(bWX - CamPosX,bWY - CamPosY,bWZ - CamPosZ,&bP1x,&bP1y);

        if(bP1x>=0){g1x[h] = (int)bP1x;}
        if(bP1y>=0){g1y[h] = (int)bP1y;}
        if(bP2x>=0){g2x[h] = (int)bP2x;}
        if(bP2y>=0){g2y[h] = (int)bP2y;}

        //drawpoint(&output2, (int)bP2x, (int)bP2y, 255, h, screenWidth, screenHeight);
        //drawpoint(&output, (int)bP1x, (int)bP1y, 255, h, screenWidth, screenHeight);

    }

    for (int ii = 0 ; ii < screenWidth ; ii++){
        for (int jj = 0; jj < screenHeight ; jj++){
            for(int kk = 0 ; kk < 15 ; kk++){
                for(int ll = 0 ; ll < 15 ; ll++){
                    int A = g1y[17*kk+ll]-g1y[17*kk+ll+1];
                    int B = g1x[17*kk+ll+1]-g1x[17*kk+ll];
                    int C = -(A*g1x[17*kk+ll] + B*g1y[17*kk+ll]);
                    int D = A*ii+B*jj+C;

                    int AA = g1y[17*kk+ll]-g1y[17*(kk+1)+ll];
                    int BB = g1x[17*(kk+1)+ll]-g1x[17*kk+ll];
                    int CC = -(AA*g1x[17*kk+ll] + BB*g1y[17*kk+ll]);
                    int DD = AA*ii+BB*jj+CC;

                    int AAA = g1y[17*(kk+1)+ll]-g1y[17*(kk+1)+ll+1];
                    int BBB = g1x[17*(kk+1)+ll+1]-g1x[17*(kk+1)+ll];
                    int CCC = -(AAA*g1x[17*(kk+1)+ll] + BBB*g1y[17*(kk+1)+ll]);
                    int DDD = AAA*ii+BBB*jj+CCC;

                    int AAAA = g1y[17*(kk+1)+ll+1]-g1y[17*kk+ll+1];
                    int BBBB = g1x[17*kk+ll+1]-g1x[17*(kk+1)+ll+1];
                    int CCCC = -(AAAA*g1x[17*(kk+1)+ll+1] + BBBB*g1y[17*(kk+1)+ll+1]);
                    int DDDD = AAAA*ii+BBBB*jj+CCCC;
                    //cout << "D, DD, DDD, DDDD" << D << "," << DD << "," << DDD << "," << DDDD << endl;
                    if(D<=0 && DD>=0 && DDD>=0 && DDDD>=0
                            && g1x[17*kk+ll] != 0
                            && g1x[17*kk+ll+1] != 0
                            && g1x[17*(kk+1)+ll] != 0
                            && g1x[17*(kk+1)+ll+1] != 0
                            && g1y[17*kk+ll] != 0
                            && g1y[17*kk+ll+1] != 0
                            && g1y[17*(kk+1)+ll] != 0
                            && g1y[17*(kk+1)+ll+1] != 0
                            ){
                        int col = 17*kk+ll;
                        //   cout << "col " << col << endl;
                        //output.SetPixel(col,ii,jj,2);
                        regGrid1.SetPixel(col,ii,jj);
                    }


                    int A1 = g2y[17*kk+ll]-g2y[17*kk+ll+1];
                    int B1 = g2x[17*kk+ll+1]-g2x[17*kk+ll];
                    int C1 = -(A1*g2x[17*kk+ll] + B1*g2y[17*kk+ll]);
                    int D1 = A1*ii+B1*jj+C1;

                    int AA1 = g2y[17*kk+ll]-g2y[17*(kk+1)+ll];
                    int BB1 = g2x[17*(kk+1)+ll]-g2x[17*kk+ll];
                    int CC1 = -(AA1*g2x[17*kk+ll] + BB1*g2y[17*kk+ll]);
                    int DD1 = AA1*ii+BB1*jj+CC1;

                    int AAA1 = g2y[17*(kk+1)+ll]-g2y[17*(kk+1)+ll+1];
                    int BBB1 = g2x[17*(kk+1)+ll+1]-g2x[17*(kk+1)+ll];
                    int CCC1 = -(AAA1*g2x[17*(kk+1)+ll] + BBB1*g2y[17*(kk+1)+ll]);
                    int DDD1 = AAA1*ii+BBB1*jj+CCC1;

                    int AAAA1 = g2y[17*(kk+1)+ll+1]-g2y[17*kk+ll+1];
                    int BBBB1 = g2x[17*kk+ll+1]-g2x[17*(kk+1)+ll+1];
                    int CCCC1 = -(AAAA1*g2x[17*(kk+1)+ll+1] + BBBB1*g2y[17*(kk+1)+ll+1]);
                    int DDDD1 = AAAA1*ii+BBBB1*jj+CCCC1;
                    //cout << "D, DD, DDD, DDDD" << D << "," << DD << "," << DDD << "," << DDDD << endl;
                    if(D1<=0 && DD1>=0 && DDD1>=0 && DDDD1>=0
                            && g2x[17*kk+ll] != 0
                            && g2x[17*kk+ll+1] != 0
                            && g2x[17*(kk+1)+ll] != 0
                            && g2x[17*(kk+1)+ll+1] != 0
                            && g2y[17*kk+ll] != 0
                            && g2y[17*kk+ll+1] != 0
                            && g2y[17*(kk+1)+ll] != 0
                            && g2y[17*(kk+1)+ll+1] != 0
                            ){
                        int col = 17*kk+ll;
                        //   cout << "col " << col << endl;
                        //output2.SetPixel(col,ii,jj,2);
                        regGrid2.SetPixel(col,ii,jj);
                    }


                }
            }
        }
    }

    sprintf(buf, "regGrid1.tiff", 2);
    vpcoreio::WriteImage(regGrid1, buf);

    sprintf(buf, "regGrid2.tiff", 2);
    vpcoreio::WriteImage(regGrid2, buf);




    double *object_x = new double[totalnumberofpixels];
    double *object_y = new double[totalnumberofpixels];
    double *object_z = new double[totalnumberofpixels];
    int *screen1_x = new int[totalnumberofpixels];
    int *screen1_y = new int[totalnumberofpixels];
    int *screen2_x = new int[totalnumberofpixels];
    int *screen2_y = new int[totalnumberofpixels];
    int *blacklist1_x = new int[totalnumberofpixels];
    int *blacklist1_y = new int[totalnumberofpixels];
    int *blacklist2_x = new int[totalnumberofpixels];
    int *blacklist2_y = new int[totalnumberofpixels];
    bool *globallyAssigned = new bool[totalnumberofpixels];

    for (int init = 0; init < totalnumberofpixels; init++){

        object_x[init] = 0;
        object_y[init] = 0;
        object_z[init] = 0;
        screen1_x[init] = 0;
        screen1_y[init] = 0;
        screen2_x[init] = 0;
        screen2_y[init] = 0;
        blacklist1_x[init] = 0;
        blacklist1_y[init] = 0;
        blacklist2_x[init] = 0;
        blacklist2_y[init] = 0;
        globallyAssigned[init] = false;

    }

    int zzz = 0;
    int bl1 = 0;
    int bl2 = 0;
    bool CamOneTrigger = false;
    bool CamTwoTrigger = false;
    int blackTol = 10;
    int blackTol2 = 50;

    bbcvp::CPicture<unsigned char> picBefBuf = picseq.GetPicture();
    bbcvp::CPicture<unsigned char> picBefBuf2 = picseq2.GetPicture();

    // TODO: add some parallelisation here?
    for( frame_count=0; ; ++frame_count ) {

        cout << "term.readChrOrEscSeq(stdin) beginning " << term.readChrOrEscSeq(stdin) << endl;

        int key;
        int key_was_pressed = false;
        while ( (key=term.readChrOrEscSeq(stdin)) != -1)
        {
          key_was_pressed = true ;
        }

        cout <<"key was pressed top " << key_was_pressed << endl;


        err = picseq.LoadNextPicture();
        err2 = picseq2.LoadNextPicture();
        if(err!=bbcvp::OK) {
            // if end of sequence is reached, program doesn't return error!
            if (frame_count == frames_to_process) {
                std::cout<<"Done! processed frames: "<<frame_count<<std::endl;
                err = bbcvp::OK;
            } else {
                std::cerr<<"Err: picseq.LoadNextPicture() returned: "<<
                           err<<", frame_count="<<frame_count<<
                           ", frames_to_process="<<frames_to_process<<
                           ", seq_len="<<seq_len<<std::endl;
            }
            break;
        }

        if(err2!=bbcvp::OK) {
            // if end of sequence is reached, program doesn't return error!
            if (frame_count == frames_to_process) {
                std::cout<<"Done! processed frames: "<<frame_count<<std::endl;
                err2 = bbcvp::OK;
            } else {
                std::cerr<<"Err: picseq.LoadNextPicture() returned: "<<
                           err2<<", frame_count="<<frame_count<<
                           ", frames_to_process="<<frames_to_process<<
                           ", seq_len="<<seq_len<<std::endl;
            }
            break;
        }



        bbcvp::CPicture<unsigned char> picBuf = picseq.GetPicture();
        bbcvp::CPicture<unsigned char> prev2mask(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> prevmap(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> golfmask(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> golfmap(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> golfsat(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> flagmap(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> flagspot(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> golfdiff(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> valuemap(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> satmask(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> huemap(screenWidth, screenHeight,3);
        bbcvp::CPicture<unsigned char> output(screenWidth, screenHeight,3);



        bbcvp::CPicture<unsigned char> picBuf2 = picseq2.GetPicture();
        bbcvp::CPicture<unsigned char> prev2mask2(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> prevmap2(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> golfmask2(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> golfmap2(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> golfsat2(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> flagmap2(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> flagspot2(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> golfdiff2(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> valuemap2(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> satmask2(screenWidth, screenHeight,1);
        bbcvp::CPicture<unsigned char> huemap2(screenWidth, screenHeight,3);
        bbcvp::CPicture<unsigned char> output2(screenWidth, screenHeight,3);

        //segment chrome on Cam One - returns coordinates of possible ball objects


        segmenter(picBuf,
                  cam1green,
                  golfscreen_x,
                  golfscreen_y,
                  isObject,
                  *buf1,
                  para.start,
                  frame_count,
                  para.end,
                  &fieldmap,
                  &golfmask,
                  &golfmap,
                  &golfsat,
                  &flagmap,
                  &flagspot,
                  &golfdiff,
                  &valuemap,
                  &satmask,
                  &huemap,
                  &output
                  );

        //segment chrome on Cam Two - returns coordinates of possible ball objects



        segmenter(picBuf2,
                  cam2green,
                  Twogolfscreen_x,
                  Twogolfscreen_y,
                  isObject2,
                  *buf2,
                  para.start,
                  frame_count,
                  para.end,
                  &fieldmapTwo,
                  &golfmask2,
                  &golfmap2,
                  &golfsat2,
                  &flagmap2,
                  &flagspot2,
                  &golfdiff2,
                  &valuemap2,
                  &satmask2,
                  &huemap2,
                  &output2
                  );



        if(frame_count != 0){
        // Motion segmentation of Cam One - returns mask

        motion_segmenter(picBuf,
                         picBefBuf,
                         cam1green,
                         &prev2mask
                         );
        // Motion segmentation of Cam Two - returns mask



        motion_segmenter(picBuf2,
                         picBefBuf2,
                         cam2green,
                         &prev2mask2
                         );
        }



        // Combine motion masks with chrome masks to make sure moving objects are balls

        for(int xm = 1;xm<picBuf.GetNx()-1;xm++)
        {
            for(int ym = 1;ym<picBuf.GetNy()-1;ym++)
            {
                if((     prev2mask.GetPixel(xm,ym)==255 && golfmask.GetPixel(xm,ym)==255)   ||
                        (prev2mask.GetPixel(xm,ym)==255 && golfmask.GetPixel(xm+1,ym)==255) ||
                        (prev2mask.GetPixel(xm,ym)==255 && golfmask.GetPixel(xm-1,ym)==255) ||
                        (prev2mask.GetPixel(xm,ym)==255 && golfmask.GetPixel(xm,ym+1)==255) ||
                        (prev2mask.GetPixel(xm,ym)==255 && golfmask.GetPixel(xm,ym-1)==255)){
                    prev2mask.SetPixel(255,xm,ym);
                }
                else{
                    prev2mask.SetPixel(0,xm,ym);
                }
                if((     prev2mask2.GetPixel(xm,ym)==255 && golfmask2.GetPixel(xm,ym)==255)   ||
                        (prev2mask2.GetPixel(xm,ym)==255 && golfmask2.GetPixel(xm+1,ym)==255) ||
                        (prev2mask2.GetPixel(xm,ym)==255 && golfmask2.GetPixel(xm-1,ym)==255) ||
                        (prev2mask2.GetPixel(xm,ym)==255 && golfmask2.GetPixel(xm,ym+1)==255) ||
                        (prev2mask2.GetPixel(xm,ym)==255 && golfmask2.GetPixel(xm,ym-1)==255)){
                    prev2mask2.SetPixel(255,xm,ym);
                }
                else{
                    prev2mask2.SetPixel(0,xm,ym);
                }
            }
        }

        // Segment corrected motion mask in Cam One - returns coordinates of objects

        uber_segmenter(prev2mask,
                       M1golfscreen_x,
                       M1golfscreen_y,
                       M1isObject,
                       &prevmap
                       );

        // Segment corrected motion mask in Cam Two - returns coordinates of objects

        uber_segmenter(prev2mask2,
                       M2golfscreen_x,
                       M2golfscreen_y,
                       M2isObject,
                       &prevmap2
                       );

        // Count number of moving objects in Cam One and Cam Two

        int NumOneOb = 0;
        int NumTwoOb = 0;

        int MV1cnt = 0;
        int MV2cnt = 0;

        while(M1isObject[MV1cnt] == true){
            cout << "MV1 " << M1golfscreen_x[MV1cnt] << "," << M1golfscreen_y[MV1cnt] << endl;
            MV1cnt +=1;
        }

        while(M2isObject[MV2cnt] == true){
            cout << "MV2 " << M2golfscreen_x[MV2cnt] << "," << M2golfscreen_y[MV2cnt] << endl;
            MV2cnt +=1;
        }

        // Work out the smallest number of objects in either movement mask - you can't have any more objects than the minimum!

        int smallMovCounter = 0;
        if(MV1cnt>MV2cnt){
            smallMovCounter = MV2cnt;
        }
        else{
            smallMovCounter = MV1cnt;
        }

        //Activate trigger on newly detected moving object in Cam One that isn't a leaf and assume ball isn't moving in first frame

        if(zzz==0){
            NumOneOb = MV1cnt;
        }
        if(MV1cnt > NumOneOb){
            CamOneTrigger = true;
            NumOneOb = MV1cnt;
        }

        //Activate trigger on newly detected moving object in Cam One that isn't a leaf and assume ball isn't moving in the first frame

        if(zzz==0){
            NumTwoOb = MV2cnt;
        }
        if(MV2cnt > NumTwoOb){
            CamTwoTrigger = true;
            NumTwoOb = MV2cnt;
        }

        min_flag = 0;

        int object_Counter = 0;
        int object_Counter2 = 0;

        // Count Objects in Chrome masks

        while(isObject[object_Counter] == true){
            object_Counter += 1;
        }

        while(isObject2[object_Counter2] == true){
            object_Counter2 += 1;
        }

        // Work out the smallest number of objects in either Chrome mask - you can't have any more objects than the minimum!

        int smallestCounter = 0;
        if(object_Counter>object_Counter2){
            smallestCounter = object_Counter2;
        }
        else{
            smallestCounter = object_Counter;
        }

        // Create and initialise object assigning variables

        int camOneObject[smallestCounter];
        int camTwoObject[smallestCounter];
        double objectX[smallestCounter];
        double objectY[smallestCounter];
        double objectZ[smallestCounter];
        bool objectAssigned[smallestCounter];
        double minimizedValue[smallestCounter];
        int nextsmallest_camOneObject[smallestCounter];
        int nextsmallest_camTwoObject[smallestCounter];
        int nextsmallest_minimizedValue[smallestCounter];

        for (int u = 0; u < smallestCounter; u++){
            camOneObject[u] = smallestCounter+10;
            camTwoObject[u] = smallestCounter+10;
            minimizedValue[u]= smallestCounter+10;
            nextsmallest_camOneObject[u]= smallestCounter+10;
            nextsmallest_camTwoObject[u] = smallestCounter+10;
            nextsmallest_minimizedValue[u] = smallestCounter+10;
            objectAssigned[u] = false;
        }

        cout << "There are " << object_Counter << " objects" << endl;
        cout << "There are " << object_Counter2 << " objects2" << endl;

        bool object1assigned[object_Counter];
        bool object2assigned[object_Counter2];

        for (int u = 0; u < object_Counter; u++){
            object1assigned[u]=false;
        }

        for (int u = 0; u < object_Counter2; u++){
            object2assigned[u]=false;
        }

        // if zzz == 0, detect all non-objects (leaves, stones, anomalies).


        if(input == false && key_was_pressed == true){
        input = key_was_pressed;
        CamOneTrigger=false;
        CamTwoTrigger=false;
        }
        cout << "INPUT = " << input << endl;
        cout << "key was pressed " << key_was_pressed << endl;
        //input = DLLFunc(function);


        if(frame_count==0 ){
            cout << "OPTION 1 OPTION 1 OPTION 1 " << endl;
            object_sorter(     golfscreen_x,
                               golfscreen_y,
                               Twogolfscreen_x,
                               Twogolfscreen_y,
                               blacklist1_x,
                               blacklist1_y,
                               blacklist2_x,
                               blacklist2_y,
                               object_Counter,
                               object_Counter2,
                               bl1,
                               bl2,
                               CamPosX,
                               CamPosY,
                               CamPosZ,
                               TwoCamPosX,
                               TwoCamPosY,
                               TwoCamPosZ,
                               regGrid1,
                               regGrid2,
                               minimizedValue,
                               camOneObject,
                               camTwoObject,
                               nextsmallest_minimizedValue,
                               nextsmallest_camOneObject,
                               nextsmallest_camTwoObject,
                               object1assigned,
                               object2assigned,
                               zzz,
                               myPoseEst,
                               TwomyPoseEst,
                               objectX,
                               objectY,
                               objectZ,
                               smallestCounter,
                               blackTol
                               );

            //Add all objects detected to the blacklist for both camera positions

            for(int b = 0; b < object_Counter; b++){
                if(object1assigned[b]==false){
                    blacklist1_x[bl1] = golfscreen_x[b];
                    blacklist1_y[bl1] = golfscreen_y[b];
                    bl1 += 1;
                }
            }
            for(int f = 0; f < object_Counter2; f++){
                if(object2assigned[f]==false){
                    blacklist2_x[bl2] = Twogolfscreen_x[f];
                    blacklist2_y[bl2] = Twogolfscreen_y[f];
                    bl2 += 1;
                }
            }
        }
        else if((CamOneTrigger==false || CamTwoTrigger == false) && input == true){
            cout << "OPTION 2 OPTION 2 OPTION 2 " << endl;
            if(frame_count==1){
                for(int b = 0; b < NumOneOb; b++){
                    if(object1assigned[b]==false){
                        blacklist1_x[bl1] = M1golfscreen_x[b];
                        blacklist1_y[bl1] = M1golfscreen_y[b];
                        bl1 += 1;
                    }
                }
                for(int f = 0; f < NumTwoOb; f++){
                    if(object2assigned[f]==false){
                        blacklist2_x[bl2] = M2golfscreen_x[f];
                        blacklist2_y[bl2] = M2golfscreen_y[f];
                        bl2 += 1;
                    }
                }
            }
            object_sorter(     M1golfscreen_x,
                               M1golfscreen_y,
                               M2golfscreen_x,
                               M2golfscreen_y,
                               blacklist1_x,
                               blacklist1_y,
                               blacklist2_x,
                               blacklist2_y,
                               NumOneOb,
                               NumTwoOb,
                               bl1,
                               bl2,
                               CamPosX,
                               CamPosY,
                               CamPosZ,
                               TwoCamPosX,
                               TwoCamPosY,
                               TwoCamPosZ,
                               regGrid1,
                               regGrid2,
                               minimizedValue,
                               camOneObject,
                               camTwoObject,
                               nextsmallest_minimizedValue,
                               nextsmallest_camOneObject,
                               nextsmallest_camTwoObject,
                               object1assigned,
                               object2assigned,
                               zzz,
                               myPoseEst,
                               TwomyPoseEst,
                               objectX,
                               objectY,
                               objectZ,
                               smallMovCounter,
                               blackTol2
                               );
        }
        else{
            cout << "DING DING DING DING" << endl;

            // need to track object detected in movement algorithm and no others.
            object_sorter(     golfscreen_x,
                               golfscreen_y,
                               Twogolfscreen_x,
                               Twogolfscreen_y,
                               blacklist1_x,
                               blacklist1_y,
                               blacklist2_x,
                               blacklist2_y,
                               object_Counter,
                               object_Counter2,
                               bl1,
                               bl2,
                               CamPosX,
                               CamPosY,
                               CamPosZ,
                               TwoCamPosX,
                               TwoCamPosY,
                               TwoCamPosZ,
                               regGrid1,
                               regGrid2,
                               minimizedValue,
                               camOneObject,
                               camTwoObject,
                               nextsmallest_minimizedValue,
                               nextsmallest_camOneObject,
                               nextsmallest_camTwoObject,
                               object1assigned,
                               object2assigned,
                               zzz,
                               myPoseEst,
                               TwomyPoseEst,
                               objectX,
                               objectY,
                               objectZ,
                               smallestCounter,
                               blackTol
                               );
            input = false;
        }


        //cout << "bl1,bl2 " << bl1 << "," << bl2 << endl;

        int yy = 0;

        double tolerance[4];

        tolerance[0] = 0.02;
        tolerance[1] = 0.01;
        tolerance[2] = 0.5;
        tolerance[3] = 2;

        for(int c = 0 ; c < zzz ; c++){
            globallyAssigned[c]=false;
        }

        if (zzz == 0){
            cout << "iffed" << endl;
            for(int b = 0; b < smallestCounter; b++){
                if(camOneObject[b]!=smallestCounter+10 &&
                        golfscreen_x[camOneObject[b]] != 0 &&
                        golfscreen_y[camOneObject[b]] != 0 &&
                        Twogolfscreen_x[camTwoObject[b]] != 0 &&
                        Twogolfscreen_y[camTwoObject[b]] != 0
                        ){
                    object_x[yy] = objectX[b];
                    object_y[yy] = objectY[b];
                    object_z[yy] = objectZ[b];
                    screen1_x[yy] = golfscreen_x[camOneObject[b]] ;
                    screen1_y[yy] = golfscreen_y[camOneObject[b]] ;
                    screen2_x[yy] = Twogolfscreen_x[camTwoObject[b]];
                    screen2_y[yy] = Twogolfscreen_y[camTwoObject[b]];
                    objectAssigned[b] = true;
                    globallyAssigned[yy] = true;
                    yy += 1;
                    zzz += 1;
                    drawpoint(&output, (int)golfscreen_x[camOneObject[b]], (int)golfscreen_y[camOneObject[b]], 64, b, screenWidth, screenHeight);
                    drawpoint(&output2, (int)Twogolfscreen_x[camTwoObject[b]], (int)Twogolfscreen_y[camTwoObject[b]], 64, b, screenWidth, screenHeight);
                }
            }
        }

        else{
            cout << "elsed" << endl;
            for(int e = 0 ; e < 4 ; e++){
                for(int c = 0 ; c < zzz ; c++){
                    for(int b = 0; b < smallestCounter; b++){
                        if (    object_x[c] < objectX[b] + tolerance[e]
                                && object_x[c] > objectX[b] - tolerance[e]
                                && object_y[c] < objectY[b] + tolerance[e]
                                && object_y[c] > objectY[b] - tolerance[e]
                                && object_z[c] < objectZ[b] + tolerance[e]
                                && object_z[c] > objectZ[b] - tolerance[e]
                                && camOneObject[b]!=smallestCounter+10
                                && objectAssigned[b] == false
                                && globallyAssigned[c] == false
                                && M1golfscreen_x[camOneObject[b]] != 0
                                && M1golfscreen_y[camOneObject[b]] != 0
                                && M2golfscreen_x[camTwoObject[b]] != 0
                                && M2golfscreen_y[camTwoObject[b]] != 0
                                ){
                            object_x[c] = objectX[b];
                            object_y[c] = objectY[b];
                            object_z[c] = objectZ[b];
                            screen1_x[c] = M1golfscreen_x[camOneObject[b]] ;
                            screen1_y[c] = M1golfscreen_y[camOneObject[b]] ;
                            screen2_x[c] = M2golfscreen_x[camTwoObject[b]];
                            screen2_y[c] = M2golfscreen_y[camTwoObject[b]];
                            objectAssigned[b] = true;
                            globallyAssigned[c] = true;

                            cout << "object " << c <<  " spotted " << e <<  endl;
                        }
                    }
                }
            }

            ;

            int tol = 60;

            for(int b = 0; b < smallestCounter; b++){
                if (objectAssigned[b] == false && camOneObject[b]!=smallestCounter+10 ){
                    int noMatch = false;
                    for (int c = 0; c < zzz; c++){
                        if(  (   screen1_x[c] < M1golfscreen_x[camOneObject[b]] + tol
                                 && screen1_x[c] > M1golfscreen_x[camOneObject[b]] - tol
                                 && screen1_y[c] < M1golfscreen_y[camOneObject[b]] + tol
                                 && screen1_y[c] > M1golfscreen_y[camOneObject[b]] - tol) ||
                             (   screen2_x[c] < M2golfscreen_x[camTwoObject[b]] + tol
                                 && screen2_x[c] > M2golfscreen_x[camTwoObject[b]] - tol
                                 && screen2_y[c] < M2golfscreen_y[camTwoObject[b]] + tol
                                 && screen2_y[c] > M2golfscreen_y[camTwoObject[b]] - tol)
                             && globallyAssigned[c] == true
                             ){
                            noMatch = true;
                        }
                    }
                    if(noMatch == false){
                        object_x[zzz] = objectX[b];
                        object_y[zzz] = objectY[b];
                        object_z[zzz] = objectZ[b];
                        screen1_x[zzz] = M1golfscreen_x[camOneObject[b]];
                        screen1_y[zzz] = M1golfscreen_y[camOneObject[b]];
                        screen2_x[zzz] = M2golfscreen_x[camTwoObject[b]];
                        screen2_y[zzz] = M2golfscreen_y[camTwoObject[b]];
                        objectAssigned[b] = true;
                        zzz+=1;
                    }
                    noMatch = false;
                }
            }
        }

        for(int c = 0 ; c < zzz ; c++){
            if(globallyAssigned[c] == true){
                drawpoint(&output,  (int)screen1_x[c], (int)screen1_y[c], 64, c, screenWidth, screenHeight);
                drawpoint(&output2, (int)screen2_x[c], (int)screen2_y[c], 64, c, screenWidth, screenHeight);
                cout << "X,Y,Z " << object_x[c] << "," << object_y[c] << "," << object_z[c] << " " << screen1_x[c] << "," << screen1_y[c] << " " << screen2_x[c] << "," << screen2_y[c]  << endl; //These two won't be equal
            }
        }

        for(int e = 0 ; e < bl1 ; e++){
            drawpoint(&output,  (int)blacklist1_x[e], (int)blacklist1_y[e], 255, e, screenWidth, screenHeight);
        }

        for(int d = 0 ; d < bl2 ; d++){
            drawpoint(&output2, (int)blacklist2_x[d], (int)blacklist2_y[d], 255, d, screenWidth, screenHeight);
        }

        cout << "there are " << zzz << " objects" << endl;

        cout <<"This far?" << endl;

        strcpy(buff, bufft);
        strcat(buff, "output/%04d.tiff");
        sprintf(buf1, buff, (int)frame_count + para.start - imgDiff);
        vpcoreio::WriteImage(output, buf1);

        strcpy(buff, buffy);
        strcat(buff, "output/%04d.tiff");
        sprintf(buf2, buff, (int)frame_count + para.start - imgDiff);
        vpcoreio::WriteImage(output2, buf2);

        strcpy(buff, bufft);
        strcat(buff, "motion/%04d.tiff");
        sprintf(buf1, buff, (int)frame_count + para.start - imgDiff);
        vpcoreio::WriteImage(prev2mask, buf1);

        strcpy(buff, buffy);
        strcat(buff, "motion/%04d.tiff");
        sprintf(buf2, buff, (int)frame_count + para.start - imgDiff);
        vpcoreio::WriteImage(prev2mask2, buf2);

        strcpy(buff, bufft);
        strcat(buff, "golfmask/%04d.tiff");
        sprintf(buf1, buff, (int)frame_count + para.start - imgDiff);
        vpcoreio::WriteImage(golfmask, buf1);

        strcpy(buff, bufft);
        strcat(buff, "satmap/%04d.tiff");
        sprintf(buf1, buff, (int)frame_count + para.start - imgDiff);
        vpcoreio::WriteImage(golfsat, buf1);

        strcpy(buff, bufft);
        strcat(buff, "satmask/%04d.tiff");
        sprintf(buf1, buff, (int)frame_count + para.start - imgDiff);
        vpcoreio::WriteImage(satmask, buf1);

        strcpy(buff, buffy);
        strcat(buff, "satmask/%04d.tiff");
        sprintf(buf2, buff, (int)frame_count + para.start - imgDiff);
        vpcoreio::WriteImage(satmask2, buf2);

        strcpy(buff, buffy);
        strcat(buff, "satmap/%04d.tiff");
        sprintf(buf2, buff, (int)frame_count + para.start - imgDiff);
        vpcoreio::WriteImage(golfsat2, buf2);

        strcpy(buff, buffy);
        strcat(buff, "golfmask/%04d.tiff");
        sprintf(buf2, buff, (int)frame_count + para.start - imgDiff);
        vpcoreio::WriteImage(golfmask2, buf2);

        std::cout<<"...\n";

        picBefBuf = picBuf;
        picBefBuf2 = picBuf2;

        cout << "term.readChrOrEscSeq(stdin) end " << term.readChrOrEscSeq(stdin) << endl;



    }
    // Close file sequence and return result
    picseq.Close();
    picseq2.Close();

    delete coordinatearray_x;
    delete coordinatearray_y;
    delete coordinatearray_z;
    delete error_x;
    delete error_y;
    delete error_z;
    delete averageCounter;
    delete golfworld_x;
    delete golfworld_y;
    delete golfworld_z;
    delete golfscreen_x;
    delete golfscreen_y;
    delete isObject;
    delete isObject2;
    delete objectCounter;

    delete Twocoordinatearray_x;
    delete Twocoordinatearray_y;
    delete Twocoordinatearray_z;
    delete Twoerror_x;
    delete Twoerror_y;
    delete Twoerror_z;
    delete TwoaverageCounter;
    delete Twogolfworld_x;
    delete Twogolfworld_y;
    delete Twogolfworld_z;
    delete Twogolfscreen_x;
    delete Twogolfscreen_y;

    delete object_x;
    delete object_y;
    delete object_z;
    delete screen1_x;
    delete screen1_y;
    delete screen2_x;
    delete screen2_y;
    delete blacklist1_x;
    delete blacklist1_y;
    delete blacklist2_x;
    delete blacklist2_y;
    delete globallyAssigned;

    return err,err2;

}



int
main(int argc, char *argv[] )
{
    _puttCalc_parameter	para;
    //int ParseArg_puttCalc (int argc, char *argv[],_puttCalc_parameter  & para );
    if(ParseArg_puttCalc(argc,argv,para)<1) return 0;
    para.Print(stdout);
    bbcvp::init_oGeM(true);
    bbcvp::SetExitError(2);
    return ltestarg(para);
}



