// ------------------------------------------------------------------------------------------------
//  The MIT License
//  
//  Copyright (c) 2013 Tim Andersen
//  
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
//  and associated documentation files (the "Software"), to deal in the Software without
//  restriction, including without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//  
//  The above copyright notice and this permission notice shall be included in all copies or
//  substantial portions of the Software.
//  
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// TinQDemo.h

#ifndef __TINQTDEMO_H
#define __TINQTDEMO_H

// -- system includes
#include <vector>

// -- includes
#include <QWidget>
#include <QPainter>

#include <mathutil.h>
#include <socket.h>

// --------------------------------------------------------------------------------------------------------------------
// -- Forward declarations

// --------------------------------------------------------------------------------------------------------------------
// struct Line:  simple wrapper to submit a line request
// --------------------------------------------------------------------------------------------------------------------
struct tLine
{
    tLine(int32 _id, const CVector3f& _start, const CVector3f& _end, int32 _color)
    {
        id = _id;
        start = _start;
        end = _end;
        color = _color;
        expired = false;
    }

    // -- object_id is the owner of the request (if there is one)
    int32 id;
    CVector3f start;
    CVector3f end;
    int32 color;
    bool expired;
};

// --------------------------------------------------------------------------------------------------------------------
// struct tCircle:  simple wrapper to submit a circle request
// --------------------------------------------------------------------------------------------------------------------
struct tCircle
{
    tCircle(int32 _id, const CVector3f& _center, float _radius, int32 _color)
    {
        id = _id;
        center = _center;
        radius = _radius;
        color = _color;
        expired = false;
    }

    int32 id;
    CVector3f center;
    float radius;
    int32 color;
    bool expired;
};

// --------------------------------------------------------------------------------------------------------------------
// struct tCircle:  simple wrapper to submit a text request
// --------------------------------------------------------------------------------------------------------------------
struct tText
{
    tText(int32 _id, const CVector3f& _position, const char* _text, int32 _color)
    {
        id = _id;
        position = _position;
        TinScript::SafeStrcpy(text, _text, TinScript::kMaxNameLength);
        color = _color;
        expired = false;
    }

    int id;
    CVector3f position;
    char text[TinScript::kMaxNameLength];
    int32 color;
    bool expired;
};

// ====================================================================================================================
// class CDemoWidget:  The widget to handle the actual drawing of the requests, using a QPainter
// ====================================================================================================================
class CDemoWidget : public QWidget
{
    Q_OBJECT

    public:
        CDemoWidget(QWidget *parent);

        // -- draw interface
        void DrawLine(int32 id, const CVector3f& start, const CVector3f& end, int color);
        void DrawCircle(int32 id, const CVector3f& center, float radius, int color);
        void DrawText(int32 id, const CVector3f& position, const char* _text, int color);
        void CancelDrawRequests(int draw_request_id);

    public slots:
        void OnUpdate();
        void OnInputKeyI();
        void OnInputKeyJ();
        void OnInputKeyL();
        void OnInputKeySpace();
		void OnInputKeyQ();

    protected:
        void paintEvent(QPaintEvent *event);

    private:
        QBrush mBackground;
        QFont mTextFont;

        // -- store the vectors of draw requests
        std::vector<tLine> mDrawLines;
        std::vector<tCircle> mDrawCircles;
        std::vector<tText> mDrawText;
};

// ====================================================================================================================
// class CDemoWindow:  A simple window to handle drawing lines and circles.
// ====================================================================================================================
class CDemoWindow : public QWidget
{
    Q_OBJECT;

    public:
        CDemoWindow();
        virtual ~CDemoWindow();

        int Exec();

        static const unsigned int kUpdateTime = 33;
        int32 GetSimTime() { return (mCurrentTime); }

    public slots:
        //void Update();

    private:
        // -- Qt components
        QApplication* mApp;

        // -- the console output handles the current time, and timer events to call Update()
        QTimer* mTimer;

        unsigned int mCurrentTime;
};

#endif // __TINQTDEMO_H

// ====================================================================================================================
// eof
// ====================================================================================================================
