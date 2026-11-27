extends CanvasLayer
## Бутони на екрана за таблет/телефон. Работят с няколко пръста едновременно.

var buttons := {}   # име -> TouchScreenButton


func _ready() -> void:
	layer = 5
	_make("left", "steer_left", "◀", 70, Color(1, 1, 1, 0.22))
	_make("right", "steer_right", "▶", 70, Color(1, 1, 1, 0.22))
	_make("gas", "accelerate", "ГАЗ", 74, Color(0.3, 0.85, 0.4, 0.3))
	_make("brake", "brake", "СТОП", 62, Color(0.95, 0.3, 0.25, 0.3))
	_make("hand", "handbrake", "Р", 38, Color(1, 1, 1, 0.18))
	_make("camera", "camera", "КАМЕРА", 38, Color(1, 1, 1, 0.18))
	_make("reset", "reset_truck", "ИЗПРАВИ", 38, Color(1, 1, 1, 0.18))
	_make("interact", "interact", "ТОВАР", 56, Color(1.0, 0.75, 0.2, 0.55))
	buttons["interact"].visible = false
	get_viewport().size_changed.connect(_layout)
	_layout()


func _circle(radius: int, color: Color) -> ImageTexture:
	var size := radius * 2
	var img := Image.create(size, size, false, Image.FORMAT_RGBA8)
	var c := Vector2(radius, radius)
	for y in size:
		for x in size:
			var d := Vector2(x + 0.5, y + 0.5).distance_to(c)
			if d <= radius:
				var col := color
				if d > radius - 4:
					col = Color(1, 1, 1, minf(color.a + 0.35, 1.0))   # светъл кант
				img.set_pixel(x, y, col)
	return ImageTexture.create_from_image(img)


func _make(id: String, action: String, text: String, radius: int, color: Color) -> void:
	var b := TouchScreenButton.new()
	b.texture_normal = _circle(radius, color)
	b.texture_pressed = _circle(radius, Color(color.r, color.g, color.b, minf(color.a + 0.3, 0.9)))
	b.action = action
	b.passby_press = true
	var lbl := Label.new()
	lbl.text = text
	lbl.size = Vector2(radius * 2, radius * 2)
	lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	lbl.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	lbl.add_theme_font_size_override("font_size", 30 if radius >= 60 else 18)
	lbl.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.8))
	lbl.add_theme_constant_override("outline_size", 6)
	lbl.mouse_filter = Control.MOUSE_FILTER_IGNORE
	b.add_child(lbl)
	add_child(b)
	buttons[id] = b


func _layout() -> void:
	var s := get_viewport().get_visible_rect().size
	var m := 28.0
	buttons["left"].position = Vector2(m, s.y - 140 - m)
	buttons["right"].position = Vector2(m + 170, s.y - 140 - m)
	buttons["gas"].position = Vector2(s.x - 148 - m, s.y - 148 - m - 60)
	buttons["brake"].position = Vector2(s.x - 148 - m - 150, s.y - 124 - m)
	buttons["hand"].position = Vector2(s.x - 76 - m - 20, s.y - 76 - m + 10)
	buttons["camera"].position = Vector2(s.x - 76 - m, 150)
	buttons["reset"].position = Vector2(s.x - 76 - m, 240)
	buttons["interact"].position = Vector2(s.x / 2 - 56, s.y - 112 - m - 110)


## Показва бутона с даден надпис ("" = скрит)
func set_interact(text: String) -> void:
	var b: TouchScreenButton = buttons["interact"]
	b.visible = text != ""
	if text != "":
		(b.get_child(0) as Label).text = text
