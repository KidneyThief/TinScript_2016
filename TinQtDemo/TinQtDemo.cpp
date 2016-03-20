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

// -- tinscript includes
#include <cmdshell.h>
#include <registrationexecs.h>

// -- includes
#include "TinQtDemo.h"

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

#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

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

    // -- We don't have a lineedit, or another standard widget to receive keypresses, so use shortcuts
    QShortcut* input_key_i = new QShortcut(QKeySequence("i"), this);
    QObject::connect(input_key_i, SIGNAL(activated()), this, SLOT(OnInputKeyI()));

    QShortcut* input_key_j = new QShortcut(QKeySequence("j"), this);
    QObject::connect(input_key_j, SIGNAL(activated()), this, SLOT(OnInputKeyJ()));

    QShortcut* input_key_l = new QShortcut(QKeySequence("l"), this);
    QObject::connect(input_key_l, SIGNAL(activated()), this, SLOT(OnInputKeyL()));

    QShortcut* input_key_space = new QShortcut(QKeySequence(" "), this);
    QObject::connect(input_key_space, SIGNAL(activated()), this, SLOT(OnInputKeySpace()));

	QShortcut* input_key_q = new QShortcut(QKeySequence("q"), this);
	QObject::connect(input_key_q, SIGNAL(activated()), this, SLOT(OnInputKeyQ()));
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
// -- input handlers (would be simpler if QWidget would receive keyPressEvent() calls...?)
static uint32 hash_NotifyEvent = TinScript::Hash("NotifyEvent");
void CDemoWidget::OnInputKeyI()
{
    int32 dummy = 0;
    TinScript::ExecFunction(dummy, hash_NotifyEvent, (int32)'i');
}
void CDemoWidget::OnInputKeyJ()
{
    int32 dummy = 0;
    TinScript::ExecFunction(dummy, hash_NotifyEvent, (int32)'j');
}
void CDemoWidget::OnInputKeyL()
{
    int32 dummy = 0;
    TinScript::ExecFunction(dummy, hash_NotifyEvent, (int32)'l');
}
void CDemoWidget::OnInputKeySpace()
{
    int32 dummy = 0;
    TinScript::ExecFunction(dummy, hash_NotifyEvent, (int32)' ');
}

void CDemoWidget::OnInputKeyQ()
{
	int32 dummy = 0;
	TinScript::ExecFunction(dummy, hash_NotifyEvent, (int32)'q');
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
            TinScript::SafeStrcpy(item.text, _text, TinScript::kMaxNameLength);
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
            break;
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
    TinScript::CScriptContext* thread_context = TinScript::CScriptContext::Create(printf, CmdShellAssertHandler);

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

void DrawText(int32 id, CVector3f position, const char* text, int32 color)
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

REGISTER_FUNCTION_P4(DrawLine, DrawLine, void, int32, CVector3f, CVector3f, int32);
REGISTER_FUNCTION_P4(DrawCircle, DrawCircle, void, int32, CVector3f, float32, int32);
REGISTER_FUNCTION_P4(DrawText, DrawText, void, int32, CVector3f, const char*, int32);

REGISTER_FUNCTION_P1(CancelDrawRequests, CancelDrawRequests, void, int32);

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

REGISTER_FUNCTION_P0(SimPause, SimPause, void);
REGISTER_FUNCTION_P0(SimUnpause, SimUnpause, void);
REGISTER_FUNCTION_P0(SimIsPaused, SimIsPaused, bool8);
REGISTER_FUNCTION_P0(GetSimTime, GetSimTime, int32);
REGISTER_FUNCTION_P1(SimSetTimeScale, SimSetTimeScale, void, float);

// --------------------------------------------------------------------------------------------------------------------
#include "TinQtDemoMoc.cpp"

// --------------------------------------------------------------------------------------------------------------------
// eof
// --------------------------------------------------------------------------------------------------------------------
