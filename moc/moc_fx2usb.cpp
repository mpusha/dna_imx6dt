/****************************************************************************
** Meta object code from reading C++ file 'fx2usb.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.5.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../v1_01_07/fx2usb.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'fx2usb.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_TFx2Usb_t {
    QByteArrayData data[8];
    char stringdata0[85];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TFx2Usb_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TFx2Usb_t qt_meta_stringdata_TFx2Usb = {
    {
QT_MOC_LITERAL(0, 0, 7), // "TFx2Usb"
QT_MOC_LITERAL(1, 8, 13), // "sendFX2Arrive"
QT_MOC_LITERAL(2, 22, 0), // ""
QT_MOC_LITERAL(3, 23, 11), // "sendFX2Left"
QT_MOC_LITERAL(4, 35, 13), // "sendUSBArrive"
QT_MOC_LITERAL(5, 49, 11), // "sendUSBLeft"
QT_MOC_LITERAL(6, 61, 12), // "getFX2Arrive"
QT_MOC_LITERAL(7, 74, 10) // "getFX2Left"

    },
    "TFx2Usb\0sendFX2Arrive\0\0sendFX2Left\0"
    "sendUSBArrive\0sendUSBLeft\0getFX2Arrive\0"
    "getFX2Left"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TFx2Usb[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   44,    2, 0x06 /* Public */,
       3,    0,   45,    2, 0x06 /* Public */,
       4,    0,   46,    2, 0x06 /* Public */,
       5,    0,   47,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   48,    2, 0x08 /* Private */,
       7,    0,   49,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void TFx2Usb::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        TFx2Usb *_t = static_cast<TFx2Usb *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->sendFX2Arrive(); break;
        case 1: _t->sendFX2Left(); break;
        case 2: _t->sendUSBArrive(); break;
        case 3: _t->sendUSBLeft(); break;
        case 4: _t->getFX2Arrive(); break;
        case 5: _t->getFX2Left(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (TFx2Usb::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TFx2Usb::sendFX2Arrive)) {
                *result = 0;
            }
        }
        {
            typedef void (TFx2Usb::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TFx2Usb::sendFX2Left)) {
                *result = 1;
            }
        }
        {
            typedef void (TFx2Usb::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TFx2Usb::sendUSBArrive)) {
                *result = 2;
            }
        }
        {
            typedef void (TFx2Usb::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&TFx2Usb::sendUSBLeft)) {
                *result = 3;
            }
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject TFx2Usb::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_TFx2Usb.data,
      qt_meta_data_TFx2Usb,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *TFx2Usb::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TFx2Usb::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_TFx2Usb.stringdata0))
        return static_cast<void*>(const_cast< TFx2Usb*>(this));
    return QObject::qt_metacast(_clname);
}

int TFx2Usb::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void TFx2Usb::sendFX2Arrive()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void TFx2Usb::sendFX2Left()
{
    QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}

// SIGNAL 2
void TFx2Usb::sendUSBArrive()
{
    QMetaObject::activate(this, &staticMetaObject, 2, Q_NULLPTR);
}

// SIGNAL 3
void TFx2Usb::sendUSBLeft()
{
    QMetaObject::activate(this, &staticMetaObject, 3, Q_NULLPTR);
}
QT_END_MOC_NAMESPACE
