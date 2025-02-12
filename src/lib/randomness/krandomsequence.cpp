/*
  This file is part of the KDE libraries
  Copyright (c) 1999 Sean Harmer <sh@astro.keele.ac.uk>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "krandomsequence.h"
#include "krandom.h"

class Q_DECL_HIDDEN KRandomSequence::Private
{
public:
    enum {SHUFFLE_TABLE_SIZE = 32};

    void draw(); // Generate the random number

    int lngSeed1;
    int lngSeed2;
    int lngShufflePos;
    int shuffleArray[SHUFFLE_TABLE_SIZE];
};

//////////////////////////////////////////////////////////////////////////////
//  Construction / Destruction
//////////////////////////////////////////////////////////////////////////////

KRandomSequence::KRandomSequence(long lngSeed1) : d(new Private)
{
    // Seed the generator
    setSeed(lngSeed1);
}

KRandomSequence::KRandomSequence(int lngSeed1) : d(new Private)
{
    // Seed the generator
    setSeed(lngSeed1);
}

KRandomSequence::~KRandomSequence()
{
    delete d;
}

KRandomSequence::KRandomSequence(const KRandomSequence &a) : d(new Private)
{
    *d = *a.d;
}

KRandomSequence &KRandomSequence::operator=(const KRandomSequence &a)
{
    if (this != &a) {
        *d = *a.d;
    }
    return *this;
}

//////////////////////////////////////////////////////////////////////////////
//  Member Functions
//////////////////////////////////////////////////////////////////////////////
void KRandomSequence::setSeed(long lngSeed1)
{
    setSeed(static_cast<int>(lngSeed1));
}

void KRandomSequence::setSeed(int lngSeed1)
{
    // Convert the positive seed number to a negative one so that the draw()
    // function can initialise itself the first time it is called. We just have
    // to make sure that the seed used != 0 as zero perpetuates itself in a
    // sequence of random numbers.
    if (lngSeed1 < 0) {
        d->lngSeed1 = -1;
    } else if (lngSeed1 == 0) {
        d->lngSeed1 = -((KRandom::random() & ~1) + 1);
    } else {
        d->lngSeed1 = -lngSeed1;
    }
}

static const int sMod1           = 2147483563;
static const int sMod2           = 2147483399;

void KRandomSequence::Private::draw()
{
    static const int sMM1            = sMod1 - 1;
    static const int sA1             = 40014;
    static const int sA2             = 40692;
    static const int sQ1             = 53668;
    static const int sQ2             = 52774;
    static const int sR1             = 12211;
    static const int sR2             = 3791;
    static const int sDiv            = 1 + sMM1 / SHUFFLE_TABLE_SIZE;

    // Long period (>2 * 10^18) random number generator of L'Ecuyer with
    // Bayes-Durham shuffle and added safeguards. Returns a uniform random
    // deviate between 0.0 and 1.0 (exclusive of the endpoint values). Call
    // with a negative number to initialize; thereafter, do not alter idum
    // between successive deviates in a sequence. RNMX should approximate
    // the largest floating point value that is less than 1.

    int j; // Index for the shuffle table
    int k;

    // Initialise
    if (lngSeed1 <= 0) {
        lngSeed2 = lngSeed1;

        // Load the shuffle table after 8 warm-ups
        for (j = SHUFFLE_TABLE_SIZE + 7; j >= 0; --j) {
            k = lngSeed1 / sQ1;
            lngSeed1 = sA1 * (lngSeed1 - k * sQ1) - k * sR1;
            if (lngSeed1 < 0) {
                lngSeed1 += sMod1;
            }

            if (j < SHUFFLE_TABLE_SIZE) {
                shuffleArray[j] = lngSeed1;
            }
        }

        lngShufflePos = shuffleArray[0];
    }

    // Start here when not initializing

    // Compute lngSeed1 = ( lngIA1*lngSeed1 ) % lngIM1 without overflows
    // by Schrage's method
    k = lngSeed1 / sQ1;
    lngSeed1 = sA1 * (lngSeed1 - k * sQ1) - k * sR1;
    if (lngSeed1 < 0) {
        lngSeed1 += sMod1;
    }

    // Compute lngSeed2 = ( lngIA2*lngSeed2 ) % lngIM2 without overflows
    // by Schrage's method
    k = lngSeed2 / sQ2;
    lngSeed2 = sA2 * (lngSeed2 - k * sQ2) - k * sR2;
    if (lngSeed2 < 0) {
        lngSeed2 += sMod2;
    }

    j = lngShufflePos / sDiv;
    lngShufflePos = shuffleArray[j] - lngSeed2;
    shuffleArray[j] = lngSeed1;

    if (lngShufflePos < 1) {
        lngShufflePos += sMM1;
    }
}

void
KRandomSequence::modulate(int i)
{
    d->lngSeed2 -= i;
    if (d->lngSeed2 < 0) {
        d->lngShufflePos += sMod2;
    }
    d->draw();
    d->lngSeed1 -= i;
    if (d->lngSeed1 < 0) {
        d->lngSeed1 += sMod1;
    }
    d->draw();
}

double
KRandomSequence::getDouble()
{
    static const double finalAmp         = 1.0 / double(sMod1);
    static const double epsilon          = 1.2E-7;
    static const double maxRand          = 1.0 - epsilon;
    double temp;
    d->draw();
    // Return a value that is not one of the endpoints
    if ((temp = finalAmp * d->lngShufflePos) > maxRand) {
        // We don't want to return 1.0
        return maxRand;
    } else {
        return temp;
    }
}

unsigned long
KRandomSequence::getLong(unsigned long max)
{
    return getInt(static_cast<int>(max));
}

unsigned int
KRandomSequence::getInt(unsigned int max)
{
    d->draw();

    return max ? ((static_cast<unsigned int>(d->lngShufflePos)) % max) : 0;
}

bool
KRandomSequence::getBool()
{
    d->draw();

    return ((static_cast<unsigned int>(d->lngShufflePos)) & 1);
}
