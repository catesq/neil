from neil.utils import Sizes, Vec2

router_sizes = Sizes(
    # pluginwidth = 120,
    # pluginheight = 30,
    gap = 4,
    bar = 12,  # the width/height of the bars used for led, cpu and panning

    # ledwidth = 'bar',
    # ledheight = 'pluginheight - gap * 2',  # size of led
    
    # cpuwidth = 'bar',
    # cpuheight = 'pluginheight - gap * 2',  # size of led

    # panwidth = 'pluginwidth - gap * 2',
    # panheight = 'bar',
    arrowradius = 8,
    # quantizex = 'pluginwidth + arrowradius * 2',
    # quantizey = 'pluginheight + arrowradius * 2',
    # volbarwidth = 32,
    # volbarheight = 128,
    # volknobheight = 16,

    plugin = Vec2(128, 32),
    quantize = Vec2('pluginwidth + arrowradius * 2', 'pluginheight + arrowradius * 2'),

    led = Vec2(6, 'plugin.y - gap * 2'),
    led_offs = Vec2('gap', 'gap'),

    cpu = Vec2(6, 'plugin.y - gap * 2'),
    cpu_offs = Vec2('pluginwidth - gap', 'gap'),

    pan = Vec2('pluginwidth - gap * 4 + bar * 2', 'bar'),
    pan_offs = Vec2('gap*2 + bar', 'pluginheight - gap - bar'),

    vol_bar = Vec2(32, 128),
    vol_knob = Vec2(16,16),
)


def draw_line(ctx, linepen, crx, cry, rx, ry):
    if abs(crx - rx) < 1 and abs(cry - ry) < 1:
        return
    
    ctx.move_to(crx, cry)
    ctx.line_to(rx, ry)
    ctx.set_source_rgb(*linepen)
    ctx.stroke()


# draw a line that looks vaguely like a sine wave along a stright line from (c_x, c_y) to (d_x, d_y) 
def draw_wavy_line(ctx, linepen, crx, cry, rx, ry, wave_height = 10, wave_length = 20):
    dx = rx - crx
    dy = ry - cry

    if abs(dx) < 1 and abs(dy) < 1:
        return

    line_len = (dx ** 2 + dy ** 2) ** 0.5

    ctx.save()
    # set up a clip region that to cut off the end of the wavy line 
    clip_expand = line_len / wave_height
    clip_offsetx = dy / clip_expand
    clip_offsety = -dx / clip_expand

    ctx.move_to(crx + clip_offsetx, cry + clip_offsety)
    ctx.line_to(rx + clip_offsetx, ry + clip_offsety)
    ctx.line_to(rx - clip_offsetx, ry - clip_offsety)
    ctx.line_to(crx - clip_offsetx, cry - clip_offsety)
    ctx.close_path()
    ctx.clip()
 
    # data used for handles of bezier curve
    curve_count = line_len / wave_length
    curve_offset = wave_length / wave_height

    xstep = dx / curve_count
    ystep = dy / curve_count

    curve_x = ystep / curve_offset
    curve_y = xstep / curve_offset

    ## curve_count + 1, draws the wavy line with an extra curve - the last wave is partly clipped  
    for i in range(0, int(curve_count) + 1):
        start_x = crx + i * xstep
        start_y = cry + i * ystep
        ctx.move_to(start_x, start_y)
        ctx.curve_to(start_x + xstep/2 - curve_x, start_y + ystep/2 + curve_y, # control point 1
                     start_x + xstep/2 + curve_x, start_y + ystep/2 - curve_y, # control point 2
                     start_x + xstep            , start_y + ystep)             # end point

    ctx.set_source_rgb(*linepen)
    ctx.stroke()
    ctx.restore()




def draw_line_arrow(bmpctx, clr, crx, cry, rx, ry, cfg):
    vx, vy = (rx - crx), (ry - cry)
    length = (vx * vx + vy * vy) ** 0.5
    if not length:
        return
    vx, vy = vx / length, vy / length

    cpx, cpy = crx + vx * (length * 0.5), cry + vy * (length * 0.5)

    def make_triangle(radius):
        ux, uy = vx, vy
        if cfg.get_curve_arrows():
            # bezier curve tangent
            def dp(t, a, b, c, d):
                return -3 * (1 - t) ** 2 * a + 3 * (1 - t) ** 2 * b - 6 * t * (1 - t) * b - 3 * (t ** 2) * c + 6 * t * (1 - t) * c + 3 * (t ** 2) * d
            tx = dp(.5, crx, crx + vx * (length * 0.6), rx - vx * (length * 0.6), rx)
            ty = dp(.5, cry, cry, ry, ry)
            tl = (tx ** 2 + ty ** 2) ** .5
            ux, uy = tx / tl, ty / tl

        t1 = (int(cpx - ux * radius + uy * radius),
                int(cpy - uy * radius - ux * radius))
        t2 = (int(cpx + ux * radius),
                int(cpy + uy * radius))
        t3 = (int(cpx - ux * radius - uy * radius),
                int(cpy - uy * radius + ux * radius))

        return t1, t2, t3

    def draw_triangle(t1, t2, t3):
        bmpctx.move_to(*t1)
        bmpctx.line_to(*t2)
        bmpctx.line_to(*t3)
        bmpctx.close_path()

    tri1 = make_triangle(SIZE.ARROWRADIUS)

    bmpctx.save()
    bmpctx.translate(-0.5, -0.5)

    bgbrush = cfg.get_float_color("MV Background")
    linepen = cfg.get_float_color("MV Line")

    # curve
    if cfg.get_curve_arrows():
        # bezier curve tanget
        def dp(t, a, b, c, d):
            return -3 * (1 - t) ** 2 * a + 3 * (1 - t) ** 2 * b - 6 * t * (1 - t) * b - 3 * (t ** 2) * c + 6 * t * (1 - t) * c + 3 * (t ** 2) * d
        tx = dp(.5, crx, crx + vx * (length * 0.6), rx - vx * (length * 0.6), rx)
        ty = dp(.5, cry, cry, ry, ry)
        tl = (tx ** 2 + ty ** 2) ** .5
        tx, ty = tx / tl, ty / tl

        # stroke the triangle
        draw_triangle(*tri1)
        bmpctx.set_source_rgb(*[x * 0.5 for x in bgbrush])
        bmpctx.set_line_width(1)
        bmpctx.stroke()

        bmpctx.move_to(crx, cry)
        bmpctx.curve_to(crx + vx * (length * 0.6), cry,
                        rx - vx * (length * 0.6), ry,
                        rx, ry)

        bmpctx.set_line_width(4)
        bmpctx.stroke_preserve()
        bmpctx.set_line_width(2.5)
        bmpctx.set_source_rgb(*clr[0])
        # bmpctx.set_source_rgb(*linepen)
        bmpctx.stroke()

        draw_triangle(*tri1)
        bmpctx.fill()
    # straight line
    else:
        bmpctx.set_line_width(1)
        bmpctx.set_source_rgb(*linepen)
        bmpctx.move_to(crx, cry)
        bmpctx.line_to(rx, ry)
        bmpctx.stroke()

        bmpctx.set_source_rgb(*clr[0])
        draw_triangle(*tri1)
        bmpctx.fill_preserve()
        bmpctx.set_source_rgb(*linepen)
        bmpctx.stroke()

    bmpctx.restore()
