/****************************************************************************
** Meta object code from reading C++ file 'TinQtDemo.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.3.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "TinQtDemo.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TinQtDemo.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.3.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_CDemoWidget_t {
    QByteArrayData data[8];
    char stringdata[86];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CDemoWidget_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CDemoWidget_t qt_meta_stringdata_CDemoWidget = {
    {
QT_MOC_LITERAL(0, 0, 11),
QT_MOC_LITERAL(1, 12, 8),
QT_MOC_LITERAL(2, 21, 0),
QT_MOC_LITERAL(3, 22, 11),
QT_MOC_LITERAL(4, 34, 11),
QT_MOC_LITERAL(5, 46, 11),
QT_MOC_LITERAL(6, 58, 15),
QT_MOC_LITERAL(7, 74, 11)
    },
    "CDemoWidget\0OnUpdate\0\0OnInputKeyI\0"
    "OnInputKeyJ\0OnInputKeyL\0OnInputKeySpace\0"
    "OnInputKeyQ"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CDemoWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   44,    2, 0x0a /* Public */,
       3,    0,   45,    2, 0x0a /* Public */,
       4,    0,   46,    2, 0x0a /* Public */,
       5,    0,   47,    2, 0x0a /* Public */,
       6,    0,   48,    2, 0x0a /* Public */,
       7,    0,   49,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void CDemoWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        CDemoWidget *_t = static_cast<CDemoWidget *>(_o);
        switch (_id) {
        case 0: _t->OnUpdate(); break;
        case 1: _t->OnInputKeyI(); break;
        case 2: _t->OnInputKeyJ(); break;
        case 3: _t->OnInputKeyL(); break;
        case 4: _t->OnInputKeySpace(); break;
        case 5: _t->OnInputKeyQ(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject CDemoWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_CDemoWidget.data,
      qt_meta_data_CDemoWidget,  qt_static_metacall, 0, 0}
};


const QMetaObject *CDemoWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CDemoWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CDemoWidget.stringdata))
        return static_cast<void*>(const_cast< CDemoWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int CDemoWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}
struct qt_meta_stringdata_CDemoWindow_t {
    QByteArrayData data[1];
    char stringdata[12];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CDemoWindow_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CDemoWindow_t qt_meta_stringdata_CDemoWindow = {
    {
QT_MOC_LITERAL(0, 0, 11)
    },
    "CDemoWindow"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CDemoWindow[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void CDemoWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObject CDemoWindow::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_CDemoWindow.data,
      qt_meta_data_CDemoWindow,  qt_static_metacall, 0, 0}
};


const QMetaObject *CDemoWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CDemoWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CDemoWindow.stringdata))
        return static_cast<void*>(const_cast< CDemoWindow*>(this));
    return QWidget::qt_metacast(_clname);
}

int CDemoWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
