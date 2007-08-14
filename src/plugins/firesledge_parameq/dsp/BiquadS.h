/*****************************************************************************

        BiquadS.h
        Copyright (c) 2002-2006 Laurent de Soras

--- Legal stuff ---

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*Tab=3***********************************************************************/



#if ! defined (dsp_BiquadS_HEADER_INCLUDED)
#define	dsp_BiquadS_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"Biquad.h"



namespace dsp
{



// Can be inherited but is not polymorph.
class BiquadS
:	public dsp::Biquad
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	dsp::Biquad	Inherited;

						BiquadS ();
						~BiquadS () {}
	inline void		copy_filter (const BiquadS &other);

	inline void		set_sample_freq (float fs);
	inline float	get_sample_freq () const;
	inline void		set_freq (float f0);
	inline float	get_freq () const;
	inline void		set_s_eq (const float b [3], const float a [3]);
	inline void		set_s_eq (const double b [3], const double a [3]);
	inline void		get_s_eq (float b [3], float a [3]) const;
	void				transform_s_to_z ();



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	float				_s_eq_b [3];	// Coefs for numerator (zeros)
	float				_s_eq_a [3];	// Coefs for denominator (poles)



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	float				_sample_freq;	// Hz, > 0
	float				_f0;				// Hz, > 0, _f0 % (_sample_freq/2) != 0



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

						BiquadS (const BiquadS &other);
	BiquadS &		operator = (const BiquadS &other);
	bool				operator == (const BiquadS &other);
	bool				operator != (const BiquadS &other);

};	// class BiquadS



}	// namespace dsp



#include	"BiquadS.hpp"



#endif	// dsp_BiquadS_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
