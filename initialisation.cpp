void initialisation(
        double coordinatearray_x[],
        double coordinatearray_y[],
        double coordinatearray_z[],
        double error_x[],
        double error_y[],
        double error_z[],
        bbcvp::CPicture<unsigned char> *fieldmap,
        double RegOneX,
        double RegOneY,
        double RegTwoX,
        double RegTwoY,
        double RealWorldRegOneX,
        double RealWorldRegOneY,
        double RealWorldRegOneZ,
        double RealWorldRegTwoX,
        double RealWorldRegTwoY,
        double RealWorldRegTwoZ,
        int totalnumberofpixels,
        long double CamPosX,
        long double CamPosY,
        long double CamPosZ,
        bbcvp::PoseEstPanTiltHead *myPoseEst,
        bool has3Dgeometry
        ){

    int screenWidth = fieldmap->GetNx();
    int screenHeight = fieldmap->GetNy();

    cout << "HELLO HELLO AM I BEING CALLED?" << endl;

//*************CAMERA ONE*************************//

//REGISTRATION MARKS AS THEY APPEAR ON THE SCREEN



//ARRAYS FOR STORING ALL 3D WORLD POSITIONS AS THEY APPEAR ON THE SCREEN WITH ASSOCIATED ERRORS


int *averageCounter = new int[totalnumberofpixels];


//ARRAYS FOR STORING THE 3D WORLD POSITIONS OF DETECTED GOLF BALLS



//ARRAYS FOR STORING THE 2D SCREEN POSITIONS OF THE DETECTED GOLF BALLS








bbcvp::CPicture<unsigned char> fieldmapTwo(screenWidth, screenHeight,3);

int fieldType = 0;
bool bottom2top = false;

// position of registration marks in the real world - HARD CODED AT THE MOMENT




double RealXone = RealWorldRegOneX-CamPosX;
double RealYone = RealWorldRegOneY-CamPosY;
double RealZone = RealWorldRegOneZ-CamPosZ;
double RealXtwo = RealWorldRegTwoX-CamPosX;
double RealYtwo = RealWorldRegTwoY-CamPosY;
double RealZtwo = RealWorldRegTwoZ-CamPosZ;



// geometry of the scene


//Nothing here yet to deal with interlace I assume?




//Added the cam postition. I've guessed at these figures based on heightOfCameraOnTower and distanceFromFootOfFlag
myPoseEst->setCamPos(0,0,0); //666175.149	5696646.839	71.146
myPoseEst->adjustCamPos(false);

//this is only needed for doing full solutions (I think).
/* myPoseEst->setTerminationConditions( 0.1,              // rmsLimit: Stop iterating when rms error changes by less than this (default in PoseEstimator is 0.001)
                                               100, // The maximum number of iterations to perform (usually set to 100 within PoseEstimator;
                                               0.01,             // mLimit: The smallest length of the update vector allowed before iterations are stopped. (PoseEstimator default is 0.01)
                                               0.0);             // MaxRmsErrorIncrease (The max increase in RMS error allowed before solution vector is scaled down) default to 0.0*/



//added these bits which are needeed so the system has an idea of the PixHeight
double notionalPixHeight = 9.0e-6;
myPoseEst->addNewImage(fieldType, bottom2top, notionalPixHeight);
myPoseEst->clearLastImage();

double panEst, tiltEst, rollEst, fovEst, cx, cy, cz;

myPoseEst->roughEstimateCameraOrientationFOVFromPointObservations();

myPoseEst->getCamRotPTR(&panEst, &tiltEst, &rollEst);
myPoseEst->getCamFOV(&fovEst, screenHeight);

int pointIndex = 0;

pointIndex = myPoseEst->addPoint(RealXone,RealYone,RealZone);

myPoseEst->addPointObservation(-1,
                               RegOneX,
                               RegOneY ,
                               1, (double *) 0, -1, pointIndex);

myPoseEst->roughEstimateCameraOrientationFOVFromPointObservations();

myPoseEst->getCamRotPTR(&panEst, &tiltEst, &rollEst);
myPoseEst->getCamFOV(&fovEst, screenHeight);

int pointIndex2 = 0;

//corrected to pointIndex2
pointIndex2 = myPoseEst->addPoint(RealXtwo,RealYtwo,RealZtwo);

myPoseEst->addPointObservation(-1,
                               RegTwoX,
                               RegTwoY ,
                               1, (double *) 0, -1, pointIndex2);

myPoseEst->rollPitchNotCamera(false);

myPoseEst->roughEstimateCameraOrientationFOVFromPointObservations();

myPoseEst->getCamRotPTR(&panEst, &tiltEst, &rollEst);
std::cout << "PTR: " << panEst << "," << tiltEst << "," << rollEst << std::endl;

//assumption of progressive images
myPoseEst->getCamFOV(&fovEst, screenHeight);
std::cout << "FOV: " << fovEst << std::endl;

myPoseEst->getCamPos(&cx, &cy, &cz);
std::cout << "camera: (" << cx << "," << cy << "," << cz << ")" << std::endl;

char camFileName[50];
sprintf(camFileName, "%05d.cam", (1));
double camX, camY, camZ, focalLength;
myPoseEst->getCamPos(&camX, &camY, &camZ);
myPoseEst->getCamFocalLength(&focalLength);
printf("%s\n", camFileName);
double pixX, pixY;
myPoseEst->getCamPixelSize(&pixX, &pixY);
double pan, tilt, roll;
myPoseEst->getCamRotPTR(&pan, &tilt, &roll);







int csvlength = 159720;
double x[csvlength], y[csvlength], z[csvlength], XX[csvlength], YY[csvlength];

if(has3Dgeometry){

    ///////// FIRST PASS  ////////////

    string line2, line3, line4;
    ifstream myfile ("data.csv");
    if (myfile.is_open())
    {
        for (int n = 0 ; n < csvlength; n++)
        {
            char * pEnd;
            getline (myfile,line2, ',' ) ;
            z[n] = strtold(line2.c_str() , &pEnd) - CamPosZ;
            getline (myfile,line3, ',' ) ;
            x[n] = strtold(line3.c_str() , &pEnd) - CamPosX;
            getline (myfile,line4, '\n' ) ;
            y[n] = strtold(line4.c_str() , &pEnd) - CamPosY;
        }
        myfile.close();
    }

    else cout << "Unable to open file";

    for (int myPixel_x = 0 ; myPixel_x < fieldmap->GetNx(); myPixel_x++)
    {
        for (int myPixel_y = 0 ; myPixel_y < fieldmap->GetNy(); myPixel_y++)
        {
            fieldmap->SetPixel(0,myPixel_x,myPixel_y,0);
            fieldmap->SetPixel(0,myPixel_x,myPixel_y,1);
            fieldmap->SetPixel(0,myPixel_x,myPixel_y,2);
        }
    }

    double xPos = 0;
    double yPos = 0;
    for (int n = 0 ; n < csvlength; n++)
    {
        myPoseEst->projectIntoImageArbitraryPoint(x[n],y[n],z[n],&xPos, &yPos);
        XX[n] = xPos;
        YY[n] = yPos;
        int gamma = (int)(40*(11+y[n]));
        if(xPos > 0 && yPos > 0 && xPos < screenWidth && yPos < screenHeight){
            fieldmap->SetPixel(gamma,(int)xPos,(int)yPos,0);
            if(gamma<128){
                fieldmap->SetPixel(255-2*gamma,(int)xPos,(int)yPos,1);
            }
            else{
                fieldmap->SetPixel(2*gamma-255,(int)xPos,(int)yPos,1);
            }
            fieldmap->SetPixel(255-gamma,(int)xPos,(int)yPos,2);
        }
    }

    int colouredIn = 0;
    int stillBlack = 0;

    for (int myPixel_x = 0 ; myPixel_x < fieldmap->GetNx(); myPixel_x++)
    {
        for (int myPixel_y = 0 ; myPixel_y < fieldmap->GetNy(); myPixel_y++)
        {
            if(
                    fieldmap->GetPixel(myPixel_x,myPixel_y,0) == 0 &&
                    fieldmap->GetPixel(myPixel_x,myPixel_y,1) == 0 &&
                    fieldmap->GetPixel(myPixel_x,myPixel_y,2) == 0
                    ){
                stillBlack +=1;
            }
            else{
                colouredIn +=1;
            }
        }
    }


    /////////  SECOND PASS  ////////////

    int multiplicationratio = stillBlack/colouredIn + 3;


    //  INTERPOLATION #1  //

    double prevZZZ = 0;

    for (int m = 0; m < csvlength-1; m++){

        for (int u = 0; u < multiplicationratio; u++){

            double xxx = u*(x[m+1]-x[m])/multiplicationratio+x[m];
            double yyy = u*(y[m+1]-y[m])/multiplicationratio+y[m];
            double zzz = u*(z[m+1]-z[m])/multiplicationratio+z[m];

            if(m == 0 && u == 0){
                prevZZZ = zzz;
            }
            //
            if(prevZZZ-zzz>-0.002)
            {

                myPoseEst->projectIntoImageArbitraryPoint(xxx,yyy,zzz,&xPos, &yPos);

                int gamma = (int)(40*(11+y[m]));
                if(xPos > 0 && yPos > 0 && xPos < screenWidth && yPos < screenHeight){

                    int alpha = (int)xPos+(int)screenWidth*(int)yPos;

                    averageCounter[alpha] += 1;

                    double holder_x = coordinatearray_x[alpha];
                    double holder_y = coordinatearray_y[alpha];
                    double holder_z = coordinatearray_z[alpha];

                    coordinatearray_x[(int)xPos+(int)screenWidth*(int)yPos] = (xxx/(double)averageCounter[alpha]+(-1+(double)averageCounter[alpha])/(double)averageCounter[alpha]*holder_x);
                    coordinatearray_y[(int)xPos+(int)screenWidth*(int)yPos] = (yyy/(double)averageCounter[alpha]+(-1+(double)averageCounter[alpha])/(double)averageCounter[alpha]*holder_y);
                    coordinatearray_z[(int)xPos+(int)screenWidth*(int)yPos] = (zzz/(double)averageCounter[alpha]+(-1+(double)averageCounter[alpha])/(double)averageCounter[alpha]*holder_z);

                    fieldmap->SetPixel(gamma,(int)xPos,(int)yPos,0);
                    if(gamma<128){
                        fieldmap->SetPixel(255-2*gamma,(int)xPos,(int)yPos,1);
                    }
                    else{
                        fieldmap->SetPixel(2*gamma-255,(int)xPos,(int)yPos,1);
                    }
                    fieldmap->SetPixel(255-gamma,(int)xPos,(int)yPos,2);
                }
            }

            prevZZZ = zzz;
        }
    }


    for (int j = 0; j < 10; j++){

        for (int i = 0; i < 3; i++){

            for (int myPixel_x = 1 ; myPixel_x < screenWidth-1; myPixel_x++)
            {
                for (int myPixel_y = 1 ; myPixel_y < screenHeight-1; myPixel_y++)
                {
                    if(fieldmap->GetPixel(myPixel_x,myPixel_y,0) == 0 &&
                            fieldmap->GetPixel(myPixel_x,myPixel_y,1) == 0 &&
                            fieldmap->GetPixel(myPixel_x,myPixel_y,2) == 0 &&
                            fieldmap->GetPixel(myPixel_x + 1,myPixel_y,0) != 0 &&
                            fieldmap->GetPixel(myPixel_x + 1,myPixel_y,1) != 0 &&
                            fieldmap->GetPixel(myPixel_x + 1,myPixel_y,2) != 0 &&
                            fieldmap->GetPixel(myPixel_x - 1,myPixel_y,0) != 0 &&
                            fieldmap->GetPixel(myPixel_x - 1,myPixel_y,1) != 0 &&
                            fieldmap->GetPixel(myPixel_x - 1,myPixel_y,2) != 0
                            ){

                        double x1,x2,y1,y2,z1,z2;

                        x1  = coordinatearray_x[myPixel_x+screenWidth*myPixel_y-1];
                        y1  = coordinatearray_y[myPixel_x+screenWidth*myPixel_y-1];
                        z1  = coordinatearray_z[myPixel_x+screenWidth*myPixel_y-1];
                        x2  = coordinatearray_x[myPixel_x+screenWidth*myPixel_y+1];
                        y2  = coordinatearray_y[myPixel_x+screenWidth*myPixel_y+1];
                        z2  = coordinatearray_z[myPixel_x+screenWidth*myPixel_y+1];

                        coordinatearray_x[myPixel_x+screenWidth*myPixel_y] = (x1+x2)/2;
                        coordinatearray_y[myPixel_x+screenWidth*myPixel_y] = (y1+y2)/2;
                        coordinatearray_z[myPixel_x+screenWidth*myPixel_y] = (z1+z2)/2;

                        myPoseEst->projectIntoImageArbitraryPoint(coordinatearray_x[myPixel_x+screenWidth*myPixel_y],
                                                                  coordinatearray_y[myPixel_x+screenWidth*myPixel_y],
                                                                  coordinatearray_z[myPixel_x+screenWidth*myPixel_y],
                                                                  &xPos,
                                                                  &yPos);
                        int gamma = (int)(40*(11+(y1+y2)/2));

                        fieldmap->SetPixel(gamma,myPixel_x,myPixel_y,0);
                        if(gamma<128){
                            fieldmap->SetPixel(255-2*gamma,myPixel_x,myPixel_y,1);
                        }
                        else{
                            fieldmap->SetPixel(2*gamma-255,myPixel_x,myPixel_y,1);
                        }
                        fieldmap->SetPixel(255-gamma,myPixel_x,myPixel_y,2);
                    }
                }
            }

            for (int myPixel_y = 1 ; myPixel_y < screenHeight-1; myPixel_y++)
            {
                for (int myPixel_x = 1 ; myPixel_x < screenWidth-1; myPixel_x++ )
                {
                    if(fieldmap->GetPixel(myPixel_x,myPixel_y,0) == 0 &&
                            fieldmap->GetPixel(myPixel_x,myPixel_y,1) == 0 &&
                            fieldmap->GetPixel(myPixel_x,myPixel_y,2) == 0 &&
                            fieldmap->GetPixel(myPixel_x,myPixel_y+1,0) != 0 &&
                            fieldmap->GetPixel(myPixel_x,myPixel_y+1,1) != 0 &&
                            fieldmap->GetPixel(myPixel_x,myPixel_y+1,2) != 0 &&
                            fieldmap->GetPixel(myPixel_x,myPixel_y-1,0) != 0 &&
                            fieldmap->GetPixel(myPixel_x,myPixel_y-1,1) != 0 &&
                            fieldmap->GetPixel(myPixel_x,myPixel_y-1,2) != 0
                            ){

                        double x1,x2,y1,y2,z1,z2;

                        x1  = coordinatearray_x[myPixel_x+screenWidth*(myPixel_y-1)];
                        y1  = coordinatearray_y[myPixel_x+screenWidth*(myPixel_y-1)];
                        z1  = coordinatearray_z[myPixel_x+screenWidth*(myPixel_y-1)];
                        x2  = coordinatearray_x[myPixel_x+screenWidth*(myPixel_y+1)];
                        y2  = coordinatearray_y[myPixel_x+screenWidth*(myPixel_y+1)];
                        z2  = coordinatearray_z[myPixel_x+screenWidth*(myPixel_y+1)];

                        coordinatearray_x[myPixel_x+screenWidth*myPixel_y] = (x1+x2)/2;
                        coordinatearray_y[myPixel_x+screenWidth*myPixel_y] = (y1+y2)/2;
                        coordinatearray_z[myPixel_x+screenWidth*myPixel_y] = (z1+z2)/2;

                        myPoseEst->projectIntoImageArbitraryPoint(coordinatearray_x[myPixel_x+screenWidth*myPixel_y],
                                                                  coordinatearray_y[myPixel_x+screenWidth*myPixel_y],
                                                                  coordinatearray_z[myPixel_x+screenWidth*myPixel_y],
                                                                  &xPos,
                                                                  &yPos);

                        int gamma = (int)(40*(11+(y1+y2)/2));

                        fieldmap->SetPixel(gamma,myPixel_x,myPixel_y,0);
                        if(gamma<128){
                            fieldmap->SetPixel(255-2*gamma,myPixel_x,myPixel_y,1);
                        }
                        else{
                            fieldmap->SetPixel(2*gamma-255,myPixel_x,myPixel_y,1);
                        }
                        fieldmap->SetPixel(255-gamma,myPixel_x,myPixel_y,2);
                    }
                }
            }
        }

        for (int myPixel_x = 1 ; myPixel_x < screenWidth-2; myPixel_x++)
        {
            for (int myPixel_y = 1 ; myPixel_y < screenHeight-1; myPixel_y++)
            {
                if(fieldmap->GetPixel(myPixel_x,myPixel_y,0) == 0 &&
                        fieldmap->GetPixel(myPixel_x,myPixel_y,1) == 0 &&
                        fieldmap->GetPixel(myPixel_x,myPixel_y,2) == 0 &&
                        fieldmap->GetPixel(myPixel_x + 1,myPixel_y,0) == 0 &&
                        fieldmap->GetPixel(myPixel_x + 1,myPixel_y,1) == 0 &&
                        fieldmap->GetPixel(myPixel_x + 1,myPixel_y,2) == 0 &&
                        fieldmap->GetPixel(myPixel_x + 2,myPixel_y,0) != 0 &&
                        fieldmap->GetPixel(myPixel_x + 2,myPixel_y,1) != 0 &&
                        fieldmap->GetPixel(myPixel_x + 2,myPixel_y,2) != 0 &&
                        fieldmap->GetPixel(myPixel_x - 1,myPixel_y,0) != 0 &&
                        fieldmap->GetPixel(myPixel_x - 1,myPixel_y,1) != 0 &&
                        fieldmap->GetPixel(myPixel_x - 1,myPixel_y,2) != 0
                        ){

                    double x1,x2,y1,y2,z1,z2;

                    x1  = coordinatearray_x[myPixel_x+screenWidth*myPixel_y-1];
                    y1  = coordinatearray_y[myPixel_x+screenWidth*myPixel_y-1];
                    z1  = coordinatearray_z[myPixel_x+screenWidth*myPixel_y-1];
                    x2  = coordinatearray_x[myPixel_x+screenWidth*myPixel_y+2];
                    y2  = coordinatearray_y[myPixel_x+screenWidth*myPixel_y+2];
                    z2  = coordinatearray_z[myPixel_x+screenWidth*myPixel_y+2];

                    coordinatearray_x[myPixel_x+screenWidth*myPixel_y] = (x2-x1)/3+x1;
                    coordinatearray_y[myPixel_x+screenWidth*myPixel_y] = (y2-y1)/3+y1;
                    coordinatearray_z[myPixel_x+screenWidth*myPixel_y] = (z2-z1)/3+z1;
                    coordinatearray_x[myPixel_x+screenWidth*myPixel_y+1] = 2*(x2-x1)/3+x1;
                    coordinatearray_y[myPixel_x+screenWidth*myPixel_y+1] = 2*(y2-y1)/3+y1;
                    coordinatearray_z[myPixel_x+screenWidth*myPixel_y+1] = 2*(z2-z1)/3+z1;

                    myPoseEst->projectIntoImageArbitraryPoint(coordinatearray_x[myPixel_x+screenWidth*myPixel_y],
                                                              coordinatearray_y[myPixel_x+screenWidth*myPixel_y],
                                                              coordinatearray_z[myPixel_x+screenWidth*myPixel_y],
                                                              &xPos,
                                                              &yPos);

                    int gamma1 = (int)(40*(11+((y2-y1)/3+y1)));
                    int gamma2 = (int)(40*(11+(2*(y2-y1)/3+y1)));

                    fieldmap->SetPixel(gamma1,myPixel_x,myPixel_y,0);
                    if(gamma1<128){
                        fieldmap->SetPixel(255-2*gamma1,myPixel_x,myPixel_y,1);
                    }
                    else{
                        fieldmap->SetPixel(2*gamma1-255,myPixel_x,myPixel_y,1);
                    }
                    fieldmap->SetPixel(255-gamma1,myPixel_x,myPixel_y,2);

                    fieldmap->SetPixel(gamma2,myPixel_x+1,myPixel_y,0);
                    if(gamma2<128){
                        fieldmap->SetPixel(255-2*gamma2,myPixel_x+1,myPixel_y,1);
                    }
                    else{
                        fieldmap->SetPixel(2*gamma2-255,myPixel_x+1,myPixel_y,1);
                    }
                    fieldmap->SetPixel(255-gamma2,myPixel_x+1,myPixel_y,2);
                }
            }
        }

        for (int myPixel_y = 1 ; myPixel_y < screenHeight-2; myPixel_y++)
        {
            for (int myPixel_x = 1 ; myPixel_x < screenWidth-1; myPixel_x++ )
            {
                if(fieldmap->GetPixel(myPixel_x,myPixel_y,0) == 0 &&
                        fieldmap->GetPixel(myPixel_x,myPixel_y,1) == 0 &&
                        fieldmap->GetPixel(myPixel_x,myPixel_y,2) == 0 &&
                        fieldmap->GetPixel(myPixel_x,myPixel_y+1,0) == 0 &&
                        fieldmap->GetPixel(myPixel_x,myPixel_y+1,1) == 0 &&
                        fieldmap->GetPixel(myPixel_x,myPixel_y+1,2) == 0 &&
                        fieldmap->GetPixel(myPixel_x,myPixel_y+2,0) != 0 &&
                        fieldmap->GetPixel(myPixel_x,myPixel_y+2,1) != 0 &&
                        fieldmap->GetPixel(myPixel_x,myPixel_y+2,2) != 0 &&
                        fieldmap->GetPixel(myPixel_x,myPixel_y-1,0) != 0 &&
                        fieldmap->GetPixel(myPixel_x,myPixel_y-1,1) != 0 &&
                        fieldmap->GetPixel(myPixel_x,myPixel_y-1,2) != 0
                        ){

                    double x1,x2,y1,y2,z1,z2;

                    x1  = coordinatearray_x[myPixel_x+screenWidth*(myPixel_y-1)];
                    y1  = coordinatearray_y[myPixel_x+screenWidth*(myPixel_y-1)];
                    z1  = coordinatearray_z[myPixel_x+screenWidth*(myPixel_y-1)];
                    x2  = coordinatearray_x[myPixel_x+screenWidth*(myPixel_y+2)];
                    y2  = coordinatearray_y[myPixel_x+screenWidth*(myPixel_y+2)];
                    z2  = coordinatearray_z[myPixel_x+screenWidth*(myPixel_y+2)];

                    coordinatearray_x[myPixel_x+screenWidth*myPixel_y] = (x2-x1)/3+x1;
                    coordinatearray_y[myPixel_x+screenWidth*myPixel_y] = (y2-y1)/3+y1;
                    coordinatearray_z[myPixel_x+screenWidth*myPixel_y] = (z2-z1)/3+z1;
                    coordinatearray_x[myPixel_x+screenWidth*(myPixel_y+1)] = 2*(x2-x1)/3+x1;
                    coordinatearray_y[myPixel_x+screenWidth*(myPixel_y+1)] = 2*(y2-y1)/3+y1;
                    coordinatearray_z[myPixel_x+screenWidth*(myPixel_y+1)] = 2*(z2-z1)/3+z1;

                    myPoseEst->projectIntoImageArbitraryPoint(coordinatearray_x[myPixel_x+screenWidth*myPixel_y],
                                                              coordinatearray_y[myPixel_x+screenWidth*myPixel_y],
                                                              coordinatearray_z[myPixel_x+screenWidth*myPixel_y],
                                                              &xPos,
                                                              &yPos);

                    int gamma1 = (int)(40*(11+((y2-y1)/3+y1)));
                    int gamma2 = (int)(40*(11+(2*(y2-y1)/3+y1)));

                    fieldmap->SetPixel(gamma1,myPixel_x,myPixel_y,0);
                    if(gamma1<128){
                        fieldmap->SetPixel(255-2*gamma1,myPixel_x,myPixel_y,1);
                    }
                    else{
                        fieldmap->SetPixel(2*gamma1-255,myPixel_x,myPixel_y,1);
                    }
                    fieldmap->SetPixel(255-gamma1,myPixel_x,myPixel_y,2);

                    fieldmap->SetPixel(gamma2,myPixel_x,myPixel_y+1,0);
                    if(gamma2<128){
                        fieldmap->SetPixel(255-2*gamma2,myPixel_x,myPixel_y+1,1);
                    }
                    else{
                        fieldmap->SetPixel(2*gamma2-255,myPixel_x,myPixel_y+1,1);
                    }
                    fieldmap->SetPixel(255-gamma2,myPixel_x,myPixel_y+1,2);
                }
            }
        }
    }



    for (int myPixel_x = 1 ; myPixel_x < screenWidth-1; myPixel_x++)
    {
        for (int myPixel_y = 1 ; myPixel_y < screenHeight-1; myPixel_y++)
        {
            if(     fieldmap->GetPixel(myPixel_x,myPixel_y,0) != 0 &&
                    fieldmap->GetPixel(myPixel_x,myPixel_y,1) != 0 &&
                    fieldmap->GetPixel(myPixel_x,myPixel_y,2) != 0){

                int dom1 = fieldmap->GetPixel(myPixel_x,myPixel_y,0);
                int dom2 = fieldmap->GetPixel(myPixel_x+1,myPixel_y,0);


                if((float)dom1/(float)dom2*((float)dom1/(float)dom2) > 1.3 &&
                        fieldmap->GetPixel(myPixel_x+1,myPixel_y,0) != 0){

                    fieldmap->SetPixel(fieldmap->GetPixel(myPixel_x, myPixel_y,0),myPixel_x+1,myPixel_y,0);
                    fieldmap->SetPixel(fieldmap->GetPixel(myPixel_x, myPixel_y,1),myPixel_x+1,myPixel_y,1);
                    fieldmap->SetPixel(fieldmap->GetPixel(myPixel_x, myPixel_y,2),myPixel_x+1,myPixel_y,2);

                    double nought =  coordinatearray_y[(myPixel_x)+screenWidth*(myPixel_y)] ;

                    coordinatearray_y[(myPixel_x+1)+screenWidth*(myPixel_y)] = nought;
                }
            }
        }
    }

    for (int myPixel_x = 1 ; myPixel_x < fieldmap->GetNx()-1; myPixel_x++)
    {
        for (int myPixel_y = 1 ; myPixel_y < fieldmap->GetNy()-1; myPixel_y++)
        {

            error_x[myPixel_x + screenWidth*(myPixel_y)] =  sqrt((pow((coordinatearray_x[myPixel_x + screenWidth*(myPixel_y-1)] - coordinatearray_x[myPixel_x + screenWidth*(myPixel_y)]),2)+
                                                                  pow((coordinatearray_x[myPixel_x + screenWidth*(myPixel_y+1)] - coordinatearray_x[myPixel_x + screenWidth*(myPixel_y)]),2)+
                                                                  pow((coordinatearray_x[myPixel_x+1 + screenWidth*(myPixel_y)] - coordinatearray_x[myPixel_x + screenWidth*(myPixel_y)]),2)+
                                                                  pow((coordinatearray_x[myPixel_x-1 + screenWidth*(myPixel_y)] - coordinatearray_x[myPixel_x + screenWidth*(myPixel_y)]),2))/4);
            error_y[myPixel_x + screenWidth*(myPixel_y)] =  sqrt((pow((coordinatearray_y[myPixel_x + screenWidth*(myPixel_y-1)] - coordinatearray_y[myPixel_x + screenWidth*(myPixel_y)]),2)+
                                                                  pow((coordinatearray_y[myPixel_x + screenWidth*(myPixel_y+1)] - coordinatearray_y[myPixel_x + screenWidth*(myPixel_y)]),2)+
                                                                  pow((coordinatearray_y[myPixel_x+1 + screenWidth*(myPixel_y)] - coordinatearray_y[myPixel_x + screenWidth*(myPixel_y)]),2)+
                                                                  pow((coordinatearray_y[myPixel_x-1 + screenWidth*(myPixel_y)] - coordinatearray_y[myPixel_x + screenWidth*(myPixel_y)]),2))/4);
            error_z[myPixel_x + screenWidth*(myPixel_y)] =  sqrt((pow((coordinatearray_z[myPixel_x + screenWidth*(myPixel_y-1)] - coordinatearray_z[myPixel_x + screenWidth*(myPixel_y)]),2)+
                                                                  pow((coordinatearray_z[myPixel_x + screenWidth*(myPixel_y+1)] - coordinatearray_z[myPixel_x + screenWidth*(myPixel_y)]),2)+
                                                                  pow((coordinatearray_z[myPixel_x+1 + screenWidth*(myPixel_y)] - coordinatearray_z[myPixel_x + screenWidth*(myPixel_y)]),2)+
                                                                  pow((coordinatearray_z[myPixel_x-1 + screenWidth*(myPixel_y)] - coordinatearray_z[myPixel_x + screenWidth*(myPixel_y)]),2))/4);
        }
    }
}


}



