#ifndef INC_COMPUTER_VISION2
#define INC_COMPUTER_VISION2

#include <math.h>
#include "C:\Users\Nathan\Documents\Visual Studio 2012\Libraries\math_1d.h"
#include "C:\Users\Nathan\Documents\Visual Studio 2012\Libraries\math_2d.h"
#include "C:\Users\Nathan\Documents\Visual Studio 2012\Libraries\math_3d.h"
#include "C:\Users\Nathan\Documents\Visual Studio 2012\Libraries\my_stl.h"
#include "C:\Users\Nathan\Documents\Visual Studio 2012\Libraries\graphics_debug.h"
#include "C:\Users\Nathan\Documents\Visual Studio 2012\Libraries\graphics_2d.h"
#include "C:\Users\Nathan\Documents\Visual Studio 2012\Libraries\color.h"
#include <vector>

template <class sample_type = rgb>
class sample_reader {
	
	sample_type last_sample;
	sample_type* source;
	quantum_2d source_dims;
	quantum_2d bot_lef, top_rig;
	unsigned int row_start;
	quantum_2d pos;

	inline unsigned int calc_source_index(int x, int y)
	{
		return x + y * source_dims.x;
	}

public:

	sample_reader (
		sample_type* p_source, quantum_2d p_source_dims,
		quantum_2d p_bot_lef = VECT_2D_INT_ORIG, quantum_2d p_top_rig = p_source_dims - quantum_2d(1, 1))
	{
		source = p_source;
		source_dims = p_source_dims;
		bot_lef = p_bot_lef;
		top_rig = p_top_rig;
	}

	~sample_reader ()
	{
	}

	quantum_2d get_source_pos()
	{
		return pos;
	}

	bool x_cont()
	{
		return pos.x <= top_rig.x;
	}

	bool y_cont()
	{
		return pos.y <= top_rig.y;
	}

	inline sample_type get_source_sample()
	{
		return source[row_start + pos.x];
	}

	void init_x()
	{		
		pos.x = bot_lef.x + 1;
	}

	void init_y()
	{
		pos.y = bot_lef.y + 1;
		row_start = calc_source_index(0, bot_lef.y) + source_dims.x;
	}

	void advance_x()
	{
		pos.x++;
	}

	void advance_y()
	{
		pos.y++;
		row_start += source_dims.x;
	}

	point_2d pos_lef()
	{
		return point_2d(pos) + point_2d(0.0f, 0.5f);
	}

	point_2d pos_rig()
	{
		return point_2d(pos) + point_2d(1.0f, 0.5f);
	}
		
	point_2d pos_top()
	{
		return point_2d(pos) + point_2d(0.5f, 0.0f);
	}

	point_2d pos_bot()
	{
		return point_2d(pos) + point_2d(0.5f, 1.0f);
	}

	/*
	bool test_x()
	{
		return cannon.cv_float().color_percept().gamma(0.333).dist_euc_thresh(
			last_sample.cv_float().color_percept().gamma(0.333),
			(float)threshhold
		);
	}
	*/

	point_2d div_x()
	{
		return get_source_pos().cv_float() + point_2d(0.0f, -0.5f);
	}

	point_2d div_y()
	{
		return get_source_pos().cv_float() + point_2d(0.5f, -1.0f);
	}
};

void group_filter(
	draw_surface* in,
	quantum_2d bot_lef = VECT_2D_INT_ORIG, quantum_2d top_rig = quantum_2d(-1, -1), 
	float threshhold = 255.0 * 3.0 * 0.01 * 2.0) 
{
	typedef segment_2d<int> seg_type;
	typedef rgb wrk_sample_type;
	typedef rgb inp_sample_type;

	class group_seg {
	public:
		seg_type seg;
		wrk_sample_type cannon;
		pixel<int> sum;

		group_seg()
		{
		}

		group_seg(int p_x, wrk_sample_type p_cannon)
		{
			seg.a = p_x;
			cannon = p_cannon;
			sum = 0;
		}

		group_seg(seg_type p_seg, wrk_sample_type p_cannon, pixel<int> p_sum)
		{
			seg = p_seg;
			cannon = p_cannon;
			sum = p_sum;
		}

		bool test(wrk_sample_type p_sample, float p_threshhold = 5.0)
		{
			return cannon.dist_euc_thresh(p_sample, p_threshhold);
		} 
		
		wrk_sample_type mean_sample()
		{
			return sum * (1.0 / double(seg.b - seg.a));
		}
	};

	typedef std::list<group_seg> seg_list;

	class group_seg_list {

		void init_seg(int p_x)
		{
			wrk_seg = group_seg(p_x, sample);
		}

		wrk_sample_type conv_inp_sample(inp_sample_type p_sample)
		{
			return p_sample;
		}

	public:

		seg_list* working;
		seg_list* reference;
		group_seg wrk_seg;
		seg_list::iterator ref_iter;
		wrk_sample_type sample;
		int start_x;

		group_seg_list(int p_start_x)
		{
			start_x = p_start_x;
			working = new seg_list;
			reference = new seg_list;
		}

		~group_seg_list()
		{
			delete working;
			delete reference;
		}

		void set_sample(inp_sample_type p_sample)
		{
			sample = conv_inp_sample(p_sample);
			wrk_seg.sum += p_sample;
		}

		void init_row()
		{
			init_seg(start_x);

			// swap working and reference lists
			seg_list* temp = working;
			working = reference;
			reference = temp;

			// clear working list about to be used
			working->clear();
			ref_iter = reference->begin();
		}

		void cut_seg(int p_x)
		{
			wrk_seg.seg.b = p_x;
			working->push_back(wrk_seg);
			init_seg(p_x);
		}

		void end_row(int p_x)
		{
			wrk_seg.seg.b = p_x;
			working->push_back(wrk_seg);
		}

		bool test_working(float p_threshhold)
		{
			return wrk_seg.test(sample, p_threshhold);
		}

		bool test_reference(int p_x, float p_threshhold)
		{
			if (reference->empty()) {
				return false;
			} else {
				//ref_iter = reference->begin();

				//mess(ref_iter->seg.b, "seg b");

				//return true;

				while (ref_iter->seg.b < p_x) {
					++ref_iter;
				}

				return ref_iter->seg.a <= p_x && ref_iter->test(sample, p_threshhold);
			}
		}
	};

	if (top_rig.x < 0) {
		top_rig = in->get_top_rig();
	}

	sample_reader<rgb> samp((rgb*)in->get_direct_ptr(), in->get_dims(), bot_lef, top_rig);
	group_seg_list segs(bot_lef.x);

	for (samp.init_y(); samp.y_cont(); samp.advance_y()) {

		segs.init_row();

		for (samp.init_x(); samp.x_cont(); samp.advance_x()) {
			segs.set_sample(samp.get_source_sample());

			if (segs.test_reference(samp.get_source_pos().x, threshhold)) {
				show_point(samp.pos_rig(), RGB_BLUE, 2, true);
			}

			if (segs.test_working(threshhold)) {
				segs.cut_seg(samp.get_source_pos().x);
				//show_point(samp.pos_rig(), RGB_ORANGE, 2, false);
			}

		}

		for (seg_list::iterator i = segs.working->begin(); i != segs.working->end(); ++i) {
			//color_seg_horz(i->seg.a, i->seg.b, samp.get_source_pos().y, i->mean_sample().reverse());
			//show_point(point_2d(float(i->seg.a), samp.get_source_pos().y - 0.5f), RGB_YELLOW, 2, false);
		}

		segs.end_row(samp.get_source_pos().x);
	}
}

#endif // INC_COMPUTER_VISION2