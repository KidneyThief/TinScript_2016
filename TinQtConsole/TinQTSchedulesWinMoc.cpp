/****************************************************************************
** Meta object code from reading C++ file 'TinQTSchedulesWin.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.3.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "TinQTSchedulesWin.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TinQTSchedulesWin.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.3.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_CScheduleEntry_t {
    QByteArrayData data[3];
    char stringdata[36];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CScheduleEntry_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CScheduleEntry_t qt_meta_stringdata_CScheduleEntry = {
    {
QT_MOC_LITERAL(0, 0, 14),
QT_MOC_LITERAL(1, 15, 19),
QT_MOC_LITERAL(2, 35, 0)
    },
    "CScheduleEntry\0OnButtonKillPressed\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CScheduleEntry[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void CScheduleEntry::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        CScheduleEntry *_t = static_cast<CScheduleEntry *>(_o);
        switch (_id) {
        case 0: _t->OnButtonKillPressed(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject CScheduleEntry::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_CScheduleEntry.data,
      qt_meta_data_CScheduleEntry,  qt_static_metacall, 0, 0}
};


const QMetaObject *CScheduleEntry::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CScheduleEntry::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CScheduleEntry.stringdata))
        return static_cast<void*>(const_cast< CScheduleEntry*>(this));
    return QWidget::qt_metacast(_clname);
}

int CScheduleEntry::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_CDebugSchedulesWin_t {
    QByteArrayData data[3];
    char stringdata[43];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CDebugSchedulesWin_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CDebugSchedulesWin_t qt_meta_stringdata_CDebugSchedulesWin = {
    {
QT_MOC_LITERAL(0, 0, 18),
QT_MOC_LITERAL(1, 19, 22),
QT_MOC_LITERAL(2, 42, 0)
    },
    "CDebugSchedulesWin\0OnButtonRefreshPressed\0"
    ""
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CDebugSchedulesWin[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void CDebugSchedulesWin::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        CDebugSchedulesWin *_t = static_cast<CDebugSchedulesWin *>(_o);
        switch (_id) {
        case 0: _t->OnButtonRefreshPressed(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject CDebugSchedulesWin::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_CDebugSchedulesWin.data,
      qt_meta_data_CDebugSchedulesWin,  qt_static_metacall, 0, 0}
};


const QMetaObject *CDebugSchedulesWin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CDebugSchedulesWin::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CDebugSchedulesWin.stringdata))
        return static_cast<void*>(const_cast< CDebugSchedulesWin*>(this));
    return QWidget::qt_metacast(_clname);
}

int CDebugSchedulesWin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
