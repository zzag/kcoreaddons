/*
    Copyright (C) 2019 Vlad Zahorodnii <vladzzag@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KCLOCKSKEWNOTIFIER_H
#define KCLOCKSKEWNOTIFIER_H

#include <kcoreaddons_export.h>
#include <QObject>

/**
 * The KClockSkewNotifier class provides a way for monitoring system clock changes.
 *
 * The KClockSkewNotifier class makes it possible to detect discontinuous changes to
 * the system clock. Such changes are usually initiated by the user adjusting values
 * in the Date and Time KCM or calls made to functions like settimeofday().
 *
 * @since 5.64
 */
class KCOREADDONS_EXPORT KClockSkewNotifier : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)

public:
    explicit KClockSkewNotifier(QObject *parent = nullptr);
    ~KClockSkewNotifier() override;

    /**
     * Returns @c true if the notifier is active; otherwise returns @c false.
     */
    bool isActive() const;

    /**
     * Sets the active status of the clock skew notifier to @p active.
     *
     * skewed() signal won't be emitted while the notifier is inactive.
     *
     * The notifier is inactive by default.
     *
     * @see activeChanged
     */
    void setActive(bool active);

Q_SIGNALS:
    /**
     * This signal is emitted whenever the active property is changed.
     */
    void activeChanged();

    /**
     * This signal is emitted whenever the system clock is changed.
     */
    void skewed();

private:
    class Private;
    QScopedPointer<Private> d;
};

#endif // KCLOCKSKEWNOTIFIER_H
