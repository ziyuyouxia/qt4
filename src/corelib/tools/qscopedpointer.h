/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSCOPEDPOINTER_H
#define QSCOPEDPOINTER_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE
QT_MODULE(Core)

template <typename T>
struct QScopedPointerDeleter
{
    static inline void cleanup(T *pointer)
    {
        // Enforce a complete type.
        // If you get a compile error here, read the secion on forward declared
        // classes in the QScopedPointer documentation.
        typedef char IsIncompleteType[ sizeof(T) ? 1 : -1 ];
        (void) sizeof(IsIncompleteType);

        delete pointer;
    }
};

template <typename T>
struct QScopedPointerArrayDeleter
{
    static inline void cleanup(T *pointer)
    {
        // Enforce a complete type.
        // If you get a compile error here, read the secion on forward declared
        // classes in the QScopedPointer documentation.
        typedef char IsIncompleteType[ sizeof(T) ? 1 : -1 ];
        (void) sizeof(IsIncompleteType);

        delete [] pointer;
    }
};

struct QScopedPointerPodDeleter
{
    static inline void cleanup(void *pointer) { if (pointer) qFree(pointer); }
};

template <typename T, typename Cleanup = QScopedPointerDeleter<T> >
class QScopedPointer
{
#ifndef Q_CC_NOKIAX86
    typedef T *QScopedPointer:: *RestrictedBool;
#endif
public:
    explicit inline QScopedPointer(T *p = 0) : d(p)
    {
    }

    inline ~QScopedPointer()
    {
        T *oldD = this->d;
        Cleanup::cleanup(oldD);
        this->d = 0;
    }

    inline T &operator*() const
    {
        Q_ASSERT(d);
        return *d;
    }

    inline T *operator->() const
    {
        Q_ASSERT(d);
        return d;
    }

    inline bool operator==(const QScopedPointer<T, Cleanup> &other) const
    {
        return d == other.d;
    }

    inline bool operator!=(const QScopedPointer<T, Cleanup> &other) const
    {
        return d != other.d;
    }

    inline bool operator!() const
    {
        return !d;
    }

#if defined(Q_CC_NOKIAX86) || defined(Q_QDOC)
    inline operator bool() const
    {
        return isNull() ? 0 : &QScopedPointer::d;
    }
#else
    inline operator RestrictedBool() const
    {
        return isNull() ? 0 : &QScopedPointer::d;
    }
#endif

    inline T *data() const
    {
        return d;
    }

    inline bool isNull() const
    {
        return !d;
    }

    inline void reset(T *other = 0)
    {
        if (d == other)
            return;
        T *oldD = d;
        d = other;
        Cleanup::cleanup(oldD);
    }

    inline T *take()
    {
        T *oldD = d;
        d = 0;
        return oldD;
    }

    typedef T *pointer;

protected:
    T *d;

private:
    Q_DISABLE_COPY(QScopedPointer)
};

template <typename T, typename Cleanup = QScopedPointerArrayDeleter<T> >
class QScopedArrayPointer : public QScopedPointer<T, Cleanup>
{
public:
    explicit inline QScopedArrayPointer(T *p = 0)
        : QScopedPointer<T, Cleanup>(p)
    {
    }

    inline T &operator[](int i)
    {
        return this->d[i];
    }

    inline const T &operator[](int i) const
    {
        return this->d[i];
    }

    inline bool operator==(const QScopedArrayPointer<T, Cleanup> &other) const
    {
        return this->d == other.d;
    }

    inline bool operator!=(const QScopedArrayPointer<T, Cleanup> &other) const
    {
        return this->d != other.d;
    }

private:
    Q_DISABLE_COPY(QScopedArrayPointer)
};

/* Internal helper class - exposes the data through data_ptr (legacy from QShared).
   Required for some internal Qt classes, do not use otherwise. */
template <typename T, typename Cleanup = QScopedPointerDeleter<T> >
class QCustomScopedPointer : public QScopedPointer<T, Cleanup>
{
public:
    explicit inline QCustomScopedPointer(T *p = 0)
        : QScopedPointer<T, Cleanup>(p)
    {
    }

    inline T *&data_ptr()
    {
        return this->d;
    }

    inline bool operator==(const QCustomScopedPointer<T, Cleanup> &other) const
    {
        return this->d == other.d;
    }

    inline bool operator!=(const QCustomScopedPointer<T, Cleanup> &other) const
    {
        return this->d != other.d;
    }

private:
    Q_DISABLE_COPY(QCustomScopedPointer)
};

/* Internal helper class - a handler for QShared* classes, to be used in QCustomScopedPointer */
template <typename T>
class QScopedPointerSharedDeleter
{
public:
    static inline void cleanup(T *d)
    {
        if (d && !d->ref.deref())
            delete d;
    }
};

/* Internal.
   This class is basically a scoped pointer pointing to a ref-counted object
 */
template <typename T>
class QScopedSharedPointer : public QCustomScopedPointer<T, QScopedPointerSharedDeleter<T> >
{
public:
    explicit inline QScopedSharedPointer(T *p = 0)
        : QCustomScopedPointer<T, QScopedPointerSharedDeleter<T> >(p)
    {
    }

    inline void detach()
    {
        qAtomicDetach(this->d);
    }

    inline void assign(T *other)
    {
        if (this->d == other)
            return;
        if (other)
            other->ref.ref();
        T *oldD = this->d;
        this->d = other;
        QScopedPointerSharedDeleter<T>::cleanup(oldD);
    }

    inline bool operator==(const QScopedSharedPointer<T> &other) const
    {
        return this->d == other.d;
    }

    inline bool operator!=(const QScopedSharedPointer<T> &other) const
    {
        return this->d != other.d;
    }

private:
    Q_DISABLE_COPY(QScopedSharedPointer)
};

QT_END_NAMESPACE
QT_END_HEADER

#endif // QSCOPEDPOINTER_H