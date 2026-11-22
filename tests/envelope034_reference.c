/* Frozen 0.0.34 interpolation reference, commit cccce3a; do not regenerate. */
/**
 * File name: envelope.c
 * Project: Geonkick (A kick synthesizer)
 *
 * Copyright (C) 2017 Iurie Nistor
 *
 * This file is part of Geonkick.
 *
 * GeonKick is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "envelope.h"

static gkick_real linear_interpolate(gkick_real x0,
                                     gkick_real y0,
                                     gkick_real x1,
                                     gkick_real y1,
                                     gkick_real x)
{
    return y0 + (y1 - y0) * ((x - x0) / (x1 - x0));
}

static gkick_real bezier_x(gkick_real t,
                           gkick_real x0,
                           gkick_real x1,
                           gkick_real x2,
                           gkick_real x3)
{
        const gkick_real one_minus_t = 1.0f - t;
        return pow(one_minus_t, 3.0f) * x0
                + 3.0f * pow(one_minus_t, 2.0f) * t * x1
                + 3.0f * one_minus_t * pow(t, 2.0f) * x2
                + pow(t, 3.0f) * x3;
}

static gkick_real bezier_y(gkick_real t,
                           gkick_real y0,
                           gkick_real y1,
                           gkick_real y2,
                           gkick_real y3)
{
        const gkick_real one_minus_t = 1.0f - t;
        return pow(one_minus_t, 3.0f) * y0
                + 3.0f * pow(one_minus_t, 2.0f) * t * y1
                + 3.0f * one_minus_t * pow(t, 2.0f) * y2
                + pow(t, 3.0f) * y3;
}

static gkick_real find_t(const struct gkick_envelope_point *p1,
                         const struct gkick_envelope_point *p2,
                         const struct gkick_envelope_point *p3,
                         gkick_real xm)
{
        gkick_real t0 = 0.0f;
        gkick_real t1 = 1.0f;
        gkick_real t;
        const gkick_real p1_x = p1->x;
        const gkick_real p2_x = p2->x;
        const gkick_real p3_x = p3->x;
        // Binary search to find t corresponding to xm.
        while (t1 - t0 > 1e-6) {
                t = (t0 + t1) * 0.5f;
                gkick_real x_t = bezier_x(t, p1_x, p1_x, p2_x, p3_x);
                if (x_t < xm)
                        t0 = t;
                else
                        t1 = t;
        }
        return t;
}

static gkick_real bezier(struct gkick_envelope_point *p1,
                         struct gkick_envelope_point *p2,
                         struct gkick_envelope_point *p3,
                         float xm)
{
    if (fabsl(p3->x - p1->x) < DBL_EPSILON)
            return p1->y;

    gkick_real t = find_t(p1, p2, p3, xm);
    return bezier_y(t, p1->y, p1->y, p2->y, p3->y);
}

gkick_real
reference_envelope_get_value(const struct gkick_envelope* envelope,
                         gkick_real xm)
{
    if (envelope && envelope->sample_count > 1) {
        if (xm < 0 || xm > 1) return 0;
        float position = xm * (envelope->sample_count - 1);
        size_t i = (size_t)position;
        if (i >= envelope->sample_count - 1) return envelope->sample_table[envelope->sample_count - 1];
        return envelope->sample_table[i] + (envelope->sample_table[i+1] - envelope->sample_table[i]) * (position-i);
    }
    if (envelope == NULL
        || envelope->npoints < 2
        || envelope->first == NULL
        || envelope->last == NULL)
        return 0.0f;

    struct gkick_envelope_point *p = envelope->first;
    if (xm < p->x || xm > envelope->last->x)
            return 0.0f;
    else if (fabsl(xm - p->x) < DBL_EPSILON)
            return p->y;
    else if (fabsl(envelope->last->x - xm) < DBL_EPSILON)
            return envelope->last->y;

    if (envelope->npoints == 2)
        return linear_interpolate(envelope->first->x, envelope->first->y,
                                  envelope->last->x, envelope->last->y, xm);

    struct gkick_envelope_point *p1 = NULL, *p2 = NULL;
    while (p) {
            p1 = p;
            p2 = p->next;
            if (p2 && p1->x <= xm && xm <= p2->x)
                    break;
            p = p->next;
    }

    if (!p1 || !p2)
            return 0.0f;

    if (p1->is_control && p1->prev)
            return bezier(p1->prev, p1, p2, xm);

    if (p2->is_control && p2->next)
            return bezier(p1, p2, p2->next, xm);

    return linear_interpolate(p1->x, p1->y, p2->x, p2->y, xm);
}
