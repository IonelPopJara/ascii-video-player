/*
*   This is a modified version inspired by PtitGnou
*   Original Code: https://github.com/PtitGnou/AsciiVideoCPP
*/

#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <Windows.h>
#include <WinUser.h>
#include <filesystem>

using namespace cv;
using namespace std;

namespace fs = std::filesystem;

char ASCII_CHARS[] = { '@', '#', 'S', '%', '?', '*', '+', ';', ':', ',', '.' };

void ClearScreen(char fill = ' ')
{
    COORD tl = { 0, 0 };
    CONSOLE_SCREEN_BUFFER_INFO s;
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(console, &s);
    DWORD written, cells = s.dwSize.X * s.dwSize.Y;
    FillConsoleOutputCharacter(console, fill, cells, tl, &written);
    FillConsoleOutputAttribute(console, s.wAttributes, cells, tl, &written);
    SetConsoleCursorPosition(console, tl);
}

void MaximizeWindow()
{
    HWND hWnd;
    SetConsoleTitle(L"ASCII Video Player");
    hWnd = FindWindow(NULL, L"ASCII Video Player");
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD NewSBSize = GetLargestConsoleWindowSize(hOut);
    SMALL_RECT DisplayArea = { 0, 0, 0, 0 };

    SetConsoleScreenBufferSize(hOut, NewSBSize);

    DisplayArea.Right = NewSBSize.X - 1;
    DisplayArea.Bottom = NewSBSize.Y - 1;

    SetConsoleWindowInfo(hOut, TRUE, &DisplayArea);

    ShowWindow(hWnd, SW_MAXIMIZE);
}

void ChangeConsoleSize(int size)
{
    CONSOLE_FONT_INFOEX cfi{};
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = size;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
}

/*
*   This function download all the frames as GrayScale png images
*/
void DownloadFramesAsGrayScale(Mat image, int frameNumber, string parentDirName)
{
    if (frameNumber == 1)
    {
        fs::path pathToGo = fs::current_path();
        pathToGo += "\\";
        pathToGo += parentDirName;
        pathToGo += "\\";

        if (!fs::exists(parentDirName)) {
            fs::create_directory(parentDirName);
            fs::current_path(pathToGo);
        }
        else {
            fs::remove_all(pathToGo);
            fs::create_directory(parentDirName);
            fs::current_path(pathToGo);
        }

        pathToGo += "framesDir\\";

        if (!fs::exists("framesDir")) {
            fs::create_directory("framesDir");
            fs::current_path(pathToGo);
        }
        else {
            fs::remove_all(pathToGo);
            fs::create_directory("framesDir");
            fs::current_path(pathToGo);
        }
    }

    /*
    *   If you want to perfom some image proccessing
    *   Here is where you have to do it
    */

    Mat imageGray;

    cvtColor(image, imageGray, COLOR_RGB2GRAY);

    /*
    *   At some point I tried to use a negative of the image for more contrast
    */

    //bitwise_not(imageGray, imageGray);

    //imageGray.convertTo(imageGray, -1, 2, 0);

    string frameName = "frame" + to_string(frameNumber) + ".png";

    imwrite(frameName, imageGray);
}

/*
*   This function converts the frames to .txt
*/
void LoadFrame(Mat image, int frameNumber, int size)
{
    if (frameNumber == 1) { //We make sure we are in the right directory
        fs::path pathToGo = fs::current_path();
        pathToGo += "\\TextDir\\";

        if (!fs::exists("TextDir")) {
            fs::create_directory("TextDir");
            fs::current_path(pathToGo);
        }
        else {
            fs::remove_all(pathToGo);
            fs::create_directory("TextDir");
            fs::current_path(pathToGo);
        }
    }

    string finalDisplay = "";

    Mat imageResized;

    MaximizeWindow();

    ChangeConsoleSize(size);

    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int w_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int w_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    double height_ratio, width_ratio;

    height_ratio = (double)w_height / image.size().height;

    width_ratio = (double)w_width / image.size().width;

    //We resize the image based on the console's window size and the font size so that it will display full screen
    resize(image, imageResized, Size(), width_ratio, height_ratio);

    for (int y = 0; y < imageResized.rows; y++)
    {
        for (int x = 0; x < imageResized.cols; x++)
        {
            Vec3b color = imageResized.at<Vec3b>(Point(x, y));

            float colorFloat = color[0];

            if (colorFloat >= 200) { //We accentuate the black and the white to make the image better
                colorFloat += 35;
                if (colorFloat >= 255) {
                    colorFloat = 255;
                }
            }
            else if (colorFloat <= 120) {
                colorFloat -= 40;

                if (colorFloat <= 0) {
                    colorFloat = 0;
                }
            }

            /*
            *   ASCII_CHARS = ["@", "#", "S", "%", "?", "*", "+", ";", ":", ",", "."]
            */

            int colorInt = floor(colorFloat / 25);

            finalDisplay += ASCII_CHARS[colorInt];
        }

        finalDisplay += "\n";
    }

    finalDisplay.resize(finalDisplay.size() - 1);

    string fileName = "txt" + to_string(frameNumber) + ".txt";

    ofstream outfile(fileName);
    outfile << finalDisplay;
    outfile.close();

    finalDisplay.clear();
}

void DisplayFrame(int numerOfFrames, string textPath, const int frameRate, int size)
{
    string framePath;
    string veryFinalDisplay;
    string musicName;

    // Cambiar estas al comienzo quizas

    ChangeConsoleSize(size);
    MaximizeWindow();

    ClearScreen();

    int FPS = frameRate;

    //Used to make sure the video is rendering properly at its original framerate
    auto time_between_frames = chrono::microseconds(chrono::seconds(1)) / FPS;

    auto target_tp = chrono::steady_clock::now();

    for (int t = 1; t <= numerOfFrames; t++)
    {
        framePath = textPath + to_string(t) + ".txt";

        ifstream stream(framePath);

        stringstream strStream;

        strStream << stream.rdbuf();

        veryFinalDisplay = strStream.str();
        stream.close();

        printf("\r");
        printf("%s", veryFinalDisplay.c_str());
        printf("\n");

        target_tp += time_between_frames;
        this_thread::sleep_until(target_tp);
    }
}

void HandleVideo(VideoCapture cap, string videoName)
{
    char downloadFrames;
    int size = 6;

    // Remove the ".mp4" from the video's name
    videoName.resize(videoName.size() - 4);

    ClearScreen();

    if (cap.isOpened())
    {
        cout << "Filed opened succesfully!\n" << endl;
    }

    int frameRate = cap.get(CAP_PROP_FPS);
    int totalFrames = cap.get(CAP_PROP_FRAME_COUNT);
    string frameRateInput;

    cout << "Capture Properties:" << endl;
    cout << "\tFrame Rate: " << frameRate << "\t" << "| Total Frames: " << totalFrames << endl;

    if (fs::exists(videoName)) {
        std::cout << "\nYou seem to already have the image files. Do you want to re-download them ? (Y/N)" << endl;
        cin >> downloadFrames;
    }
    else {
        downloadFrames = 'Y';
    }

    // Download the frames if they are not already downloaded

    bool framesDownloaded = false;
    int frameNumber = 1;

    Mat frame;
    string progressBar;

    if (downloadFrames == 'Y') {

        cout << "Converting frames into GrayScale and downloading. This could take some time." << endl;

        while (true) {
            cap >> frame;

            if (frame.empty())
                break;

            DownloadFramesAsGrayScale(frame, frameNumber, videoName);

            if (frameNumber == 1) {
                ClearScreen();
            }

            if (frameNumber % (totalFrames / 10) == 0) {
                progressBar += ".";
                std::cout << "\rProgression : " << progressBar;
            }

            std::cout << "\rProgression : " << progressBar << " Frame " << frameNumber << " of " << totalFrames;

            frameNumber++;
        }

        std::cout << "\nDone" << endl;

        cap.release();

        framesDownloaded = true;
    }

    /*
    *   Once the image files are downloaded we convert them to .txt
    */

    char downloadTextFiles;

    // Reset frameNumber and progressBar indicator
    frameNumber = 1;
    progressBar = "";

    Mat currentFrame;
    string imagePathStr;
    fs::path videoPath;

    if (framesDownloaded) {
        videoPath = fs::current_path().parent_path();
        videoPath += "\\";
    }
    else {
        videoPath = fs::current_path();
        videoPath += "\\";
        videoPath += videoName;
        videoPath += "\\";
    }

    fs::current_path(videoPath);

    if (fs::exists("TextDir")) {
        std::cout << "You seem to already have the text files. Do you want to re-download them ? (Y/N)" << endl;
        cin >> downloadTextFiles;
    }
    else {
        downloadTextFiles = 'Y';
    }

    bool wroteCurrentFile = FALSE;

    if (downloadTextFiles == 'Y') {

        char changeSize;

        cout << "The default font size is 6. The lower it is, the better the video's quality will be. We recommend not going lower than 6, as this could cause lag on the video. If 6 is already causing lag, we recommend going up to 8 or 10. Do you want to change font size ? (Y/N)" << endl;
        cin >> changeSize;

        if (changeSize == 'Y') {
            cout << "Please enter a size: " << endl;
            cin >> size;
        }

        std::cout << "Creating text files..." << endl;
        for (int n = 1; n <= totalFrames; n++) {

            fs::path imagePath = fs::current_path();

            imagePathStr = imagePath.string();

            if (n != 1) {
                imagePathStr.resize(imagePathStr.size() - 8);
            }

            imagePathStr += "\\framesDir\\frame";
            imagePathStr += to_string(n);
            imagePathStr += ".png";


            std::replace(imagePathStr.begin(), imagePathStr.end(), '\\', '/');

            currentFrame = imread(imagePathStr);

            if (currentFrame.empty()) {
                std::cout << imagePathStr << " - Could not load image" << endl;
            }
            else {
                LoadFrame(currentFrame, frameNumber, size);
            }

            if (frameNumber % (totalFrames / 10) == 0) {
                progressBar += ".";
            }

            std::cout << "\rProgression : " << progressBar << " Frame " << frameNumber << " of " << totalFrames;
            frameNumber++;

            if (frameNumber == totalFrames) {
                break;
            }
        }

        fs::current_path(fs::current_path().parent_path());
        wroteCurrentFile = TRUE;
    }

    fs::path sizePath = fs::current_path();
    sizePath += "\\size.txt";
    int sizeInFile;

    //If the video had already been downloaded, we reuse the font size saved for it. Else, we create a new save
    if (fs::exists(sizePath)) {
        ifstream streamSize(sizePath);
        stringstream sizeStream;
        sizeStream << streamSize.rdbuf();
        sizeInFile = stoi(sizeStream.str());
        streamSize.close();

        if (wroteCurrentFile) {
            fs::remove_all(sizePath);
            ofstream outfileText("size.txt");
            outfileText << size;
            outfileText.close();
        }
        else {
            size = sizeInFile;
        }
    }
    else {
        ofstream outfileText("size.txt");
        outfileText << size;
        outfileText.close();
    }

    fs::path textPath = fs::current_path();
    string textPathStr = textPath.string();
    textPathStr += "\\TextDir\\txt";
    std::replace(textPathStr.begin(), textPathStr.end(), '\\', '/');

    std::cout << "\nAll files downloaded. Launching the video... \n";

    DisplayFrame(totalFrames, textPathStr, frameRate, size);
}

/*
    Colors Selectors for Console
    system("Color F0") means White Background, Black Text
    0 = Black        8 = Gray
    1 = Blue        9 = Light Blue
    2 = Green        A = Light Green
    3 = Blue-Gray        B = Cyan
    4 = Redd      C = Light Red
    5 = Purple     D = Light Purple
    6 = Yellow        E = Light Yellow
    7 = White       F = Clear white
    
    */

int main()
{
    //Sets the console color to white and the text to red
    system("color F0");

    string videoName;

    ClearScreen();

    std::cout << "\nWhat is the video name? Must be a valid mp4." << endl;
    cin >> videoName;

    videoName += ".mp4";

    VideoCapture cap(videoName);

    HandleVideo(cap, videoName);

    std::cout << "\nProgram Executed Succesfully" << endl;
}
