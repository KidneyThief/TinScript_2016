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
// -- input keys - special keys are 100 + key_code
const int k_maxKeyCode = 160;
#define KeyCodes(Key, Key_Entry)            \
    Key_Entry(Key, space,           32)     \
    Key_Entry(Key, A,               65)     \
    Key_Entry(Key, B,               66)     \
    Key_Entry(Key, C,               67)     \
    Key_Entry(Key, D,               68)     \
    Key_Entry(Key, E,               69)     \
    Key_Entry(Key, F,               70)     \
    Key_Entry(Key, G,               71)     \
    Key_Entry(Key, H,               72)     \
    Key_Entry(Key, I,               73)     \
    Key_Entry(Key, J,               74)     \
    Key_Entry(Key, K,               75)     \
    Key_Entry(Key, L,               76)     \
    Key_Entry(Key, M,               77)     \
    Key_Entry(Key, N,               78)     \
    Key_Entry(Key, O,               79)     \
    Key_Entry(Key, P,               80)     \
    Key_Entry(Key, Q,               81)     \
    Key_Entry(Key, R,               82)     \
    Key_Entry(Key, S,               83)     \
    Key_Entry(Key, T,               84)     \
    Key_Entry(Key, U,               85)     \
    Key_Entry(Key, V,               86)     \
    Key_Entry(Key, W,               87)     \
    Key_Entry(Key, X,               88)     \
    Key_Entry(Key, Y,               89)     \
    Key_Entry(Key, Z,               90)     \
    Key_Entry(Key, tilde,           96)     \
    Key_Entry(Key, zero,            48)     \
    Key_Entry(Key, one,             49)     \
    Key_Entry(Key, two,             50)     \
    Key_Entry(Key, three,           51)     \
    Key_Entry(Key, four,            52)     \
    Key_Entry(Key, five,            53)     \
    Key_Entry(Key, six,             54)     \
    Key_Entry(Key, seven,           55)     \
    Key_Entry(Key, eight,           56)     \
    Key_Entry(Key, nine,            57)     \
    Key_Entry(Key, minus,           45)     \
    Key_Entry(Key, equals,          61)     \
    Key_Entry(Key, backslash,       92)     \
    Key_Entry(Key, leftbracket,     91)     \
    Key_Entry(Key, rightbracket,    93)     \
    Key_Entry(Key, semicolon,       59)     \
    Key_Entry(Key, quote,           39)     \
    Key_Entry(Key, comma,           44)     \
    Key_Entry(Key, period,          46)     \
    Key_Entry(Key, forwardslash,    47)     \
    Key_Entry(Key, esc,             100)    \
    Key_Entry(Key, tab,             101)    \
    Key_Entry(Key, caps,            136)    \
    Key_Entry(Key, shift,           132)    \
    Key_Entry(Key, ctrl,            133)    \
    Key_Entry(Key, alt,             135)    \
    Key_Entry(Key, backspace,       103)    \
    Key_Entry(Key, enter,           104)    \
    Key_Entry(Key, insert,          106)    \
    Key_Entry(Key, del,             107)    \
    Key_Entry(Key, home,            116)    \
    Key_Entry(Key, end,             117)    \
    Key_Entry(Key, pageup,          122)    \
    Key_Entry(Key, pagedown,        123)    \
    Key_Entry(Key, uparrow,         119)    \
    Key_Entry(Key, downarrow,       121)    \
    Key_Entry(Key, leftarrow,       118)    \
    Key_Entry(Key, rightarrow,      120)    \
    Key_Entry(Key, f1,              148)    \
    Key_Entry(Key, f2,              149)    \
    Key_Entry(Key, f3,              150)    \
    Key_Entry(Key, f4,              151)    \
    Key_Entry(Key, f5,              152)    \
    Key_Entry(Key, f6,              153)    \
    Key_Entry(Key, f7,              154)    \
    Key_Entry(Key, f8,              155)    \
    Key_Entry(Key, f9,              156)    \
    Key_Entry(Key, f10,             157)    \
    Key_Entry(Key, f11,             158)    \
    Key_Entry(Key, f12,             159)    \

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
// struct tRect:  simple wrapper to submit a rectangle request
// --------------------------------------------------------------------------------------------------------------------
struct tRect
{
    tRect(int32 _id, const CVector3f& _pos, float _width, float _height, int32 _color)
    {
        id = _id;
        pos = _pos;
        width = _width;
        height = _height;
        color = _color;
        expired = false;
    }

    int32 id;
    CVector3f pos;
    float width;
    float height;
    int32 color;
    bool expired;
};

// --------------------------------------------------------------------------------------------------------------------
// struct tText:  simple wrapper to submit a text request
// --------------------------------------------------------------------------------------------------------------------
struct tText
{
    tText(int32 _id, const CVector3f& _position, const char* _text, int32 _color)
    {
        id = _id;
        position = _position;
        TinScript::SafeStrcpy(text, TinScript::kMaxNameLength, _text);
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
        void DrawRect(int32 id, const CVector3f& position, float width, float height, int color);
        void DrawText(int32 id, const CVector3f& position, const char* _text, int color);
        void CancelDrawRequests(int draw_request_id);

        bool8 IsKeyPressed(int32 key_code);
        bool8 KeyPressedSinceTime(int32 key_code, int32 update_time);
        bool8 KeyReleasedSinceTime(int32 key_code, int32 update_time);

    public slots:
        void OnUpdate();

    protected:
        void paintEvent(QPaintEvent *event);
        virtual void keyPressEvent(QKeyEvent* event);
        virtual void keyReleaseEvent(QKeyEvent* event);
        void UpdateKeyEvent(int32 key_code, bool8 pressed);

    private:
        QBrush mBackground;
        QFont mTextFont;

        // -- store the vectors of draw requests
        std::vector<tLine> mDrawLines;
        std::vector<tCircle> mDrawCircles;
        std::vector<tRect> mDrawRects;
        std::vector<tText> mDrawText;

        // -- track key events
        struct tKeyState
        {
            bool8 m_pressed;
            int32 m_keyTime;
        };
        tKeyState m_keyStates[k_maxKeyCode];
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
