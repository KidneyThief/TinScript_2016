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

// --------------------------------------------------------------------------------------------------------------------
// TinQtDemo.cpp : Defines the entry point for the console application, creates a simple QPainter window.
//

// -- system includes
#include "stdafx.h"

// -- Qt includes
#include <QTimer>
#include <QShortcut>
#include <QApplication>
#include <QPainter>
#include <QPaintEvent>

// -- tinscript includes
#include <cmdshell.h>
#include <TinRegBinding.h>

// -- includes
#include "TinQtDemo.h"

// -- forward declares ------------------------------------------------------------------------------------------------
int32 GetSimTime();

// --------------------------------------------------------------------------------------------------------------------
// -- override the macro from integration.h
#undef TinPrint
#define TinPrint Print

// --------------------------------------------------------------------------------------------------------------------
// -- statics
static CCmdShell* gCmdShell = NULL;
static CDemoWidget* gCanvas = NULL;

static int gCurrentTimeMS = 0;
static bool gPaused = false;
static float gTimeScale = 1.0f;

// == class CDemoWidget ===============================================================================================

CDemoWidget::CDemoWidget(QWidget *parent)
    : QWidget(parent)
{
    // -- easy access for registered functions
    gCanvas = this;

    setFixedSize(640, 480);
    setGeometry(0, 0, 640, 480);
    updateGeometry();

    QLinearGradient gradient(QPointF(50, -20), QPointF(80, 20));
    gradient.setColorAt(0.0, Qt::white);
    gradient.setColorAt(1.0, QColor(0xa6, 0xce, 0x39));

    mBackground = QBrush(QColor(64, 32, 64));
    mTextFont.setPixelSize(12);

    // -- setting focus allows the widget to receive keyPressEvent()s
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);

    // -- initialize the key states
    for (int32 i = 0; i < k_maxKeyCode; ++i)
    {
        m_keyStates[i].m_pressed = false;
        m_keyStates[i].m_keyTime = 0;
    }
}

void CDemoWidget::OnUpdate()
{
    // -- update the command shell, execute any command statement returned
    if (gCmdShell)
    {
        const char* command = gCmdShell->Update();
        if (command)
        {
            TinScript::ExecCommand(command);

            // -- once handled, refresh the prompt
            gCmdShell->RefreshConsoleInput(true, "");
        }
    }

    // -- scale the elapsed time
    int scaled_delta_ms = int(gTimeScale * float(qobject_cast<QTimer*>(sender())->interval()));
    if (!gPaused)
    {
        gCurrentTimeMS += scaled_delta_ms;
    }

    // -- update the context
    TinScript::UpdateContext(gCurrentTimeMS);

    // -- repaint the window
    repaint();
}

// --------------------------------------------------------------------------------------------------------------------
// -- input handler, sends the key press event to the script context
static uint32 hash_NotifyEvent = TinScript::Hash("NotifyEvent");
void CDemoWidget::keyPressEvent(QKeyEvent* event)
{
    // -- no repeats!
    if (event->isAutoRepeat())
        return;

    // -- calculate the key_code from the event
    int32 key_code = event->key();
    if (key_code & 0x1000000)
        key_code = (key_code & ~0x1000000) + 100;

    // -- ensure we're within range of the max key code
    if (key_code < k_maxKeyCode)
    {
        // -- do not handle key repeats
        if (!m_keyStates[key_code].m_pressed)
        {
            // -- track the key state
            UpdateKeyEvent(key_code, true);

            if (TinScript::GetContext()->FunctionExists(hash_NotifyEvent, 0))
            {
                int32 dummy = 0;
                TinScript::ExecFunction(dummy, hash_NotifyEvent, key_code, true);
            }
        }
    }
}

void CDemoWidget::keyReleaseEvent(QKeyEvent* event)
{
    // -- no repeats!
    if (event->isAutoRepeat())
        return;

    // -- calculate the key_code from the event
    int32 key_code = event->key();
    if (key_code & 0x1000000)
        key_code = (key_code & ~0x1000000) + 100;

    // -- ensure we're within range of the max key code
    if (key_code < k_maxKeyCode)
    {
        // -- track the key state
        UpdateKeyEvent(key_code, false);

        if (TinScript::GetContext()->FunctionExists(hash_NotifyEvent, 0))
        {
            int32 dummy = 0;
            TinScript::ExecFunction(dummy, hash_NotifyEvent, key_code, false);
        }
    }
}

void CDemoWidget::UpdateKeyEvent(int32 key_code, bool8 pressed)
{
    // -- sanity check
    if (key_code < 0 || key_code >= k_maxKeyCode)
        return;

    // -- timestamp the state change
    if (m_keyStates[key_code].m_pressed != pressed)
    {
        m_keyStates[key_code].m_keyTime = GetSimTime();
        m_keyStates[key_code].m_pressed = pressed;
    }
}

// --------------------------------------------------------------------------------------------------------------------
void CDemoWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(mTextFont);

    //painter.fillRect(event->rect(), mBackground);
    painter.translate(0, 0);
    painter.save();

    // -- draw all the submitted lines
    int line_count = mDrawLines.size();
    for (int i = 0; i < line_count; ++i)
    {
        tLine& line = mDrawLines[i];
        if (!line.expired)
        {
            // -- create the colored pen
            int a = (line.color >> 24) & 0xff;
            int r = (line.color >> 16) & 0xff;
            int g = (line.color >> 8) & 0xff;
            int b = (line.color >> 0) & 0xff;
            QColor color(r, g, b, a);
            QPen pen(color);

            painter.setPen(pen);
            painter.drawLine(line.start.x, line.start.y, line.end.x, line.end.y);
        }
    }

    // -- draw all the submitted circles
    int circle_count = mDrawCircles.size();
    for (int i = 0; i < circle_count; ++i)
    {
        tCircle& circle = mDrawCircles[i];
        if (!circle.expired)
        {
            // -- create the colored pen
            int a = (circle.color >> 24) & 0xff;
            int r = (circle.color >> 16) & 0xff;
            int g = (circle.color >> 8) & 0xff;
            int b = (circle.color >> 0) & 0xff;
            QColor color(r, g, b, a);
            QPen pen(color);

            QRadialGradient gradient(circle.center.x, circle.center.y, circle.radius);
            gradient.setColorAt(0.0, Qt::white);
            gradient.setColorAt(1.0f, color);

            painter.setPen(pen);
            painter.setBrush(gradient);
            painter.drawEllipse(QPoint(circle.center.x, circle.center.y), (int)circle.radius, (int)circle.radius);
        }
    }

    // -- draw all the submitted circles
    int rect_count = mDrawRects.size();
    for (int i = 0; i < rect_count; ++i)
    {
        tRect& rect = mDrawRects[i];
        if (!rect.expired)
        {
            // -- create the colored pen
            int a = (rect.color >> 24) & 0xff;
            int r = (rect.color >> 16) & 0xff;
            int g = (rect.color >> 8) & 0xff;
            int b = (rect.color >> 0) & 0xff;
            QColor color(r, g, b, a);
            QPen pen(color);

            QRadialGradient gradient(rect.pos.x, rect.pos.y, rect.width > rect.height ? rect.width : rect.height);
            gradient.setColorAt(0.0, Qt::white);
            gradient.setColorAt(1.0f, color);

            painter.setPen(pen);
            painter.setBrush(gradient);
            painter.fillRect(QRect(rect.pos.x, rect.pos.y, rect.width, rect.height), gradient);
        }
    }

    // -- draw all the submitted text
    int text_count = mDrawText.size();
    for (int i = 0; i < text_count; ++i)
    {
        tText& text = mDrawText[i];
        if (!text.expired)
        {
            // -- create the colored pen
            int a = (text.color >> 24) & 0xff;
            int r = (text.color >> 16) & 0xff;
            int g = (text.color >> 8) & 0xff;
            int b = (text.color >> 0) & 0xff;
            QColor color(r, g, b, a);
            QPen pen(color);

            QString string_text(text.text);
            painter.setPen(pen);
            QFontMetrics font_metrics(mTextFont);
            int font_string_width = font_metrics.width(string_text);
            painter.drawText(text.position.x - font_string_width / 2, text.position.y, font_string_width, 20,
                             Qt::AlignCenter, string_text);
        }
    }

    painter.restore();
    painter.end();
}

// -- drawing interface
void CDemoWidget::DrawLine(int32 id, const CVector3f& start, const CVector3f& end, int color)
{
    // -- find an expired line, to avoid thrashing memory
    bool found = false;
    int count = mDrawLines.size();
    for (int i = 0; i < count; ++i)
    {
        tLine& item = mDrawLines[i];
        if (item.expired)
        {
            item.id = id;
            item.start = start;
            item.end = end;
            item.color = color;
            item.expired = false;

            // -- we found an expired entry
            found = true;
            break;
        }
    }

    // -- if we didn't find an expired entry, add a new one
    if (!found)
    {
        tLine line(id, start, end, color);
        mDrawLines.push_back(line);
    }
}

void CDemoWidget::DrawCircle(int32 id, const CVector3f& center, float radius, int color)
{
    // -- find an expired item, to avoid thrashing memory
    bool found = false;
    int count = mDrawCircles.size();
    for (int i = 0; i < count; ++i)
    {
        tCircle& item = mDrawCircles[i];
        if (item.expired)
        {
            item.id = id;
            item.center = center;
            item.radius = radius;
            item.color = color;
            item.expired = false;

            // -- we found an expired entry
            found = true;
            break;
        }
    }

    // -- if we didn't find an expired entry, add a new one
    if (!found)
    {
        tCircle circle(id, center, radius, color);
        mDrawCircles.push_back(circle);
    }
}

void CDemoWidget::DrawRect(int32 id, const CVector3f& pos, float width, float height, int color)
{
    // -- find an expired item, to avoid thrashing memory
    bool found = false;
    int count = mDrawRects.size();
    for (int i = 0; i < count; ++i)
    {
        tRect& item = mDrawRects[i];
        if (item.expired)
        {
            item.id = id;
            item.pos = pos;
            item.width = width;
            item.height = height;
            item.color = color;
            item.expired = false;

            // -- we found an expired entry
            found = true;
            break;
        }
    }

    // -- if we didn't find an expired entry, add a new one
    if (!found)
    {
        tRect rect(id, pos, width, height, color);
        mDrawRects.push_back(rect);
    }
}

void CDemoWidget::DrawText(int32 id, const CVector3f& position, const char* _text, int color)
{
    // -- find an expired item, to avoid thrashing memory
    bool found = false;
    int count = mDrawText.size();
    for (int i = 0; i < count; ++i)
    {
        tText& item = mDrawText[i];
        if (item.expired)
        {
            item.id = id;
            item.position = position;
            TinScript::SafeStrcpy(item.text, kMaxNameLength, _text);
            item.color = color;
            item.expired = false;

            // -- we found an expired entry
            found = true;
            break;
        }
    }

    // -- if we didn't find an expired entry, add a new one
    if (!found)
    {
        tText text(id, position, _text, color);
        mDrawText.push_back(text);
    }
}

void CDemoWidget::CancelDrawRequests(int draw_request_id)
{
    // -- mark all associated lines as expired
    std::vector<tLine>::iterator line_it;
    for (line_it = mDrawLines.begin(); line_it != mDrawLines.end(); ++line_it)
    {
        tLine& item = *line_it;
        if (draw_request_id < 0 || item.id == draw_request_id)
        {
            item.expired = true;
        }
    }

    std::vector<tCircle>::iterator circle_it;
    for (circle_it = mDrawCircles.begin(); circle_it != mDrawCircles.end(); ++circle_it)
    {
        tCircle& item = *circle_it;
        if (draw_request_id < 0 || item.id == draw_request_id)
        {
            item.expired = true;
        }
    }

    std::vector<tRect>::iterator rect_it;
    for (rect_it = mDrawRects.begin(); rect_it != mDrawRects.end(); ++rect_it)
    {
        tRect& item = *rect_it;
        if (draw_request_id < 0 || item.id == draw_request_id)
        {
            item.expired = true;
        }
    }

    std::vector<tText>::iterator text_it;
    for (text_it = mDrawText.begin(); text_it != mDrawText.end(); ++text_it)
    {
        tText& item = *text_it;
        if (draw_request_id < 0 || item.id == draw_request_id)
        {
            item.expired = true;
        }
    }
}

bool8 CDemoWidget::IsKeyPressed(int32 key_code)
{
    // -- sanity check
    if (key_code < 0 || key_code >= k_maxKeyCode)
        return (false);

    return (m_keyStates[key_code].m_pressed);
}

bool8 CDemoWidget::KeyPressedSinceTime(int32 key_code, int32 update_time)
{
    // -- sanity check
    if (key_code < 0 || key_code >= k_maxKeyCode)
        return (false);

    return (m_keyStates[key_code].m_pressed && m_keyStates[key_code].m_keyTime >= update_time);
}

bool8 CDemoWidget::KeyReleasedSinceTime(int32 key_code, int32 update_time)
{
    // -- sanity check
    if (key_code < 0 || key_code >= k_maxKeyCode)
        return (false);

    return (!m_keyStates[key_code].m_pressed && m_keyStates[key_code].m_keyTime >= update_time);
}

// == class CDemoWindow ===============================================================================================

CDemoWindow::CDemoWindow() : QWidget()
{
    setWindowTitle(tr("TinScript Demo"));
    setFixedSize(640, 480);

    CDemoWidget *native = new CDemoWidget(this);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), native, SLOT(OnUpdate()));
    timer->start(10);
}

CDemoWindow::~CDemoWindow()
{
}

int CDemoWindow::Exec()
{
    return (mApp->exec());
}

// --------------------------------------------------------------------------------------------------------------------
int32 _tmain(int argc, _TCHAR* argv[])
{
    // -- required to ensure registered functions from unittest.cpp are linked.
    REGISTER_FILE(unittest_cpp);
    REGISTER_FILE(mathutil_cpp);
    REGISTER_FILE(socket_cpp);

    // -- Create the TinScript context, using the default printf, and no assert handler
    TinScript::CScriptContext* thread_context = TinScript::CScriptContext::Create(CmdShellPrintf, CmdShellAssertHandler);

    // -- create a command shell
    gCmdShell = new CCmdShell();

    // -- create a socket, so we can allow a remote debugger to connect
    SocketManager::Initialize();

    // -- create the console, and start the execution
    QApplication app(argc, argv);
    CDemoWindow demo_window;
    demo_window.show();
    int result = app.exec();

    // -- shutdown
    SocketManager::Terminate();
    TinScript::DestroyContext();

    return result;
}

// --------------------------------------------------------------------------------------------------------------------
// Registered wrappers for submitting draw requests to the Canvas
void DrawLine(int32 id, CVector3f start, CVector3f end, int32 color)
{
    if (gCanvas)
    {
        gCanvas->DrawLine(id, start, end, color);
    }
}

void DrawCircle(int32 id, CVector3f center, float32 radius, int32 color)
{
    if (gCanvas)
    {
        gCanvas->DrawCircle(id, center, radius, color);
    }
}

void DrawRect(int32 id, CVector3f position, float32 width, float32 height, int32 color)
{
    if (gCanvas)
    {
        gCanvas->DrawRect(id, position, width, height, color);
    }
}

void DrawScreenText(int32 id, CVector3f position, const char* text, int32 color)
{
    if (gCanvas)
    {
        gCanvas->DrawText(id, position, text, color);
    }
}

void CancelDrawRequests(int32 id)
{
    if (gCanvas)
    {
        gCanvas->CancelDrawRequests(id);
    }
}

REGISTER_FUNCTION(DrawLine, DrawLine);
REGISTER_FUNCTION(DrawCircle, DrawCircle);
REGISTER_FUNCTION(DrawRect, DrawRect);
REGISTER_FUNCTION(DrawText, DrawScreenText);

REGISTER_FUNCTION(CancelDrawRequests, CancelDrawRequests);

// --------------------------------------------------------------------------------------------------------------------
// Event Handlers
void ScriptNotifyEvent(int32 keypress)
{
    int32 dummy = 0;
}

void SimPause()
{
    gPaused = true;
    TinScript::SetTimeScale(0.0f);
}

void SimUnpause()
{
    gPaused = false;
    TinScript::SetTimeScale(gTimeScale);
}

bool8 SimIsPaused()
{
    return (gPaused);
}

int32 GetSimTime()
{
    return (gCurrentTimeMS);
}

void SimSetTimeScale(float scale)
{
    gTimeScale = scale;
    TinScript::SetTimeScale(scale);
}

REGISTER_FUNCTION(SimPause, SimPause);
REGISTER_FUNCTION(SimUnpause, SimUnpause);
REGISTER_FUNCTION(SimIsPaused, SimIsPaused);
REGISTER_FUNCTION(GetSimTime, GetSimTime);
REGISTER_FUNCTION(SimSetTimeScale, SimSetTimeScale);

// -- register the enum class KeyCode 
CREATE_ENUM_CLASS(KeyCode, KeyCodes);
REGISTER_ENUM_CLASS(KeyCode, KeyCodes);

bool8 IsKeyPressed(int32 key_code)
{
    if (!gCanvas)
        return (false);
    return (gCanvas->IsKeyPressed(key_code));
}

bool8 KeyPressedSinceTime(int32 key_code, int32 update_time)
{
    if (!gCanvas)
        return (false);
    return (gCanvas->KeyPressedSinceTime(key_code, update_time));
}

bool8 KeyReleasedSinceTime(int32 key_code, int32 update_time)
{
    if (!gCanvas)
        return (false);
    return (gCanvas->KeyReleasedSinceTime(key_code, update_time));
}

REGISTER_FUNCTION(IsKeyPressed, IsKeyPressed);
REGISTER_FUNCTION(KeyPressedSinceTime, KeyPressedSinceTime);
REGISTER_FUNCTION(KeyReleasedSinceTime, KeyReleasedSinceTime);

// --------------------------------------------------------------------------------------------------------------------
#include "TinQtDemoMoc.cpp"

// --------------------------------------------------------------------------------------------------------------------
// eof
// --------------------------------------------------------------------------------------------------------------------
