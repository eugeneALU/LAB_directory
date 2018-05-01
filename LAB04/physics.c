#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "physics.h"

#ifndef sqr
#define sqr(a) ((a) * (a))
#endif

#ifndef sign
#define sign(a) ((a) > 0 ? 1 : -1)
#endif

int feuler(pcord_t *a, float time_step)
{
	a->x = a->x + time_step * a->vx;
	a->y = a->y + time_step * a->vy;
	return 0;
}

float wall_collide(pcord_t *p, cord_t wall)
{
	float gPreassure = 0.0;

	if (p->x < wall.x0)
	{
		p->vx = -p->vx;
		p->x = wall.x0 + (wall.x0 - p->x);
		gPreassure += 2.0 * fabs(p->vx);
	}
	else if (p->x > wall.x1)
	{
		p->vx = -p->vx;
		p->x = wall.x1 - (p->x - wall.x1);
		gPreassure += 2.0 * fabs(p->vx);
	}
	if (p->y < wall.y0)
	{
		p->vy = -p->vy;
		p->y = wall.y0 + (wall.y0 - p->y);
		gPreassure += 2.0 * fabs(p->vy);
	}
	else if (p->y > wall.y1)
	{
		p->vy = -p->vy;
		p->y = wall.y1 - (p->y - wall.y1);
		gPreassure += 2.0 * fabs(p->vy);
	}
	return gPreassure;
}

/* return when two particle will collide in this time step(0~1), otherwise return -1 */
float collide(pcord_t *p1, pcord_t *p2)
{
	double a, b, c;
	double temp, t1, t2;

	a = sqr(p1->vx - p2->vx) + sqr(p1->vy - p2->vy);	//square of relative velocity (A^2+B^2=C^2)
	b = 2 * ((p1->x - p2->x) * (p1->vx - p2->vx) + (p1->y - p2->y) * (p1->vy - p2->vy));	//
	c = sqr(p1->x - p2->x) + sqr(p1->y - p2->y) - 4 * 1 * 1;	//square of relative position-4*1*1 (WHY -4*1*1?)

	if (a != 0.0)	//if their relative velocity is same (a=0) --> they won't collide
	{
		temp = sqr(b) - 4 * a * c;	//
		if (temp >= 0)
		{
			temp = sqrt(temp);
			t1 = (-b + temp) / (2 * a);
			t2 = (-b - temp) / (2 * a);

			if (t1 > t2)	//swap t1 t2 --> let t1 be smaller one
			{
				temp = t1;
				t1 = t2;
				t2 = temp;
			}

			if ((t1 >= 0) & (t1 <= 1))
				return t1;
			else if ((t2 >= 0) & (t2 <= 1))
				return t2;
		}
	}
	return -1;
}

void interact(pcord_t *p1, pcord_t *p2, float t)
{
	float c, s, a, b, tao;
	pcord_t p1temp, p2temp;

	if (t >= 0)
	{

		/* Move to impact point */
		(void)feuler(p1, t);
		(void)feuler(p2, t);

		/* Rotate the coordinate system around p1*/
		p2temp.x = p2->x - p1->x;
		p2temp.y = p2->y - p1->y;

		/* Givens plane rotation, Golub, van Loan p. 216 */
		/* in case overflow or downflow during caculation --> https://blog.csdn.net/yueyedeai/article/details/15217457*/ 
		a = p2temp.x;
		b = p2temp.y;
		if (p2->y == 0)	/////////weird: p2->y --> p2temp.y?
		{
			c = 1;
			s = 0;
		}
		else
		{
			if (fabs(b) > fabs(a))
			{
				tao = -a / b;
				s = 1 / (sqrt(1 + sqr(tao)));
				c = s * tao;
			}
			else
			{
				tao = -b / a;
				c = 1 / (sqrt(1 + sqr(tao)));
				s = c * tao;
			}
		}

		/* clockwise rotate =counterclock wise rotate (-Θ)					/* counterclockwise rotate (Θ)
		/* [c  s][x]   ＿  [x'] */    										/* [cosΘ  -sinΘ][x]   ＿  [x'] */
		/* [-s c][y]   ￣  [y'] */    		   							   /* [sinΘ   cosΘ][y]   ￣  [y'] */
		/* p2's location after rotation --> won't use later*/ 
		p2temp.x = c * p2temp.x + s * p2temp.y; /* This should be equal to 2r */
		p2temp.y = 0.0;

		/* p1 p2's velocity after rotation */
		p2temp.vx = c * p2->vx + s * p2->vy;
		p2temp.vy = -s * p2->vx + c * p2->vy;
		p1temp.vx = c * p1->vx + s * p1->vy;
		p1temp.vy = -s * p1->vx + c * p1->vy;

		/* calculate velocity after collision */
		/* Assume the balls has the same mass... */
		/* elastic collision --> bounced back with the original velocity*/ 
		////////// weird: shouldn't it be p1temp = -p2temp and vice varsa 
		p1temp.vx = -p1temp.vx;
		p2temp.vx = -p2temp.vx;

		/* rotate back to origin direction*/
		/* using left matrix quation above */
		p1->vx = c * p1temp.vx - s * p1temp.vy;
		p1->vy = s * p1temp.vx + c * p1temp.vy;
		p2->vx = c * p2temp.vx - s * p2temp.vy;
		p2->vy = s * p2temp.vx + c * p2temp.vy;

		/* Move the balls the remaining time. */
		c = 1.0 - t;
		(void)feuler(p1, c);
		(void)feuler(p2, c);
	}
}
