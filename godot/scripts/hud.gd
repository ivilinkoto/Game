extends CanvasLayer
## Интерфейс: пари, поръчка, скорост, лента за товарене и съобщения.

var money_label: Label
var job_label: Label
var speed_label: Label
var hint_label: Label
var toast_label: Label
var progress: ProgressBar
var toast_time := 0.0


func _panel_style(alpha := 0.55) -> StyleBoxFlat:
	var sb := StyleBoxFlat.new()
	sb.bg_color = Color(0.06, 0.08, 0.11, alpha)
	sb.set_corner_radius_all(10)
	sb.content_margin_left = 16
	sb.content_margin_right = 16
	sb.content_margin_top = 8
	sb.content_margin_bottom = 8
	return sb


func _label(size: int, color := Color.WHITE) -> Label:
	var l := Label.new()
	l.add_theme_font_size_override("font_size", size)
	l.add_theme_color_override("font_color", color)
	l.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.7))
	l.add_theme_constant_override("outline_size", 4)
	l.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return l


func _ready() -> void:
	layer = 4
	var root := Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	# Горе вляво: лого и пари
	var top_left := PanelContainer.new()
	top_left.add_theme_stylebox_override("panel", _panel_style())
	top_left.position = Vector2(20, 16)
	top_left.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var vb := VBoxContainer.new()
	vb.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var logo := _label(20, Color(0.95, 0.66, 0.23))
	logo.text = "CARGO STATE"
	vb.add_child(logo)
	money_label = _label(28)
	vb.add_child(money_label)
	top_left.add_child(vb)
	root.add_child(top_left)

	# Горе в центъра: поръчката
	var job_panel := PanelContainer.new()
	job_panel.add_theme_stylebox_override("panel", _panel_style())
	job_panel.set_anchors_preset(Control.PRESET_CENTER_TOP)
	job_panel.grow_horizontal = Control.GROW_DIRECTION_BOTH
	job_panel.position.y = 16
	job_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var jvb := VBoxContainer.new()
	jvb.mouse_filter = Control.MOUSE_FILTER_IGNORE
	job_label = _label(24)
	job_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	job_label.custom_minimum_size = Vector2(560, 0)
	job_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	jvb.add_child(job_label)
	hint_label = _label(18, Color(0.8, 0.85, 0.9))
	hint_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	hint_label.custom_minimum_size = Vector2(560, 0)
	hint_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	jvb.add_child(hint_label)
	progress = ProgressBar.new()
	progress.custom_minimum_size = Vector2(360, 18)
	progress.show_percentage = false
	progress.visible = false
	jvb.add_child(progress)
	job_panel.add_child(jvb)
	root.add_child(job_panel)

	# Долу в центъра: скорост
	speed_label = _label(44)
	speed_label.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
	speed_label.grow_horizontal = Control.GROW_DIRECTION_BOTH
	speed_label.grow_vertical = Control.GROW_DIRECTION_BEGIN
	speed_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	speed_label.position.y -= 24
	root.add_child(speed_label)

	# Съобщение в средата
	toast_label = _label(40, Color(0.55, 1.0, 0.6))
	toast_label.set_anchors_preset(Control.PRESET_CENTER)
	toast_label.grow_horizontal = Control.GROW_DIRECTION_BOTH
	toast_label.grow_vertical = Control.GROW_DIRECTION_BOTH
	toast_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	toast_label.position.y -= 120
	toast_label.visible = false
	root.add_child(toast_label)


func set_money(amount: int, deliveries: int) -> void:
	money_label.text = "$ %d   ·   курсове: %d" % [amount, deliveries]


func set_job(title: String, hint: String) -> void:
	job_label.text = title
	hint_label.text = hint


func set_speed(kmh: float) -> void:
	speed_label.text = "%d км/ч" % int(round(kmh))


func set_progress(value: float) -> void:
	progress.visible = value >= 0.0
	if value >= 0.0:
		progress.value = value * 100.0


func toast(text: String, color := Color(0.55, 1.0, 0.6)) -> void:
	toast_label.text = text
	toast_label.add_theme_color_override("font_color", color)
	toast_label.visible = true
	toast_time = 3.0


func _process(delta: float) -> void:
	if toast_time > 0.0:
		toast_time -= delta
		if toast_time <= 0.0:
			toast_label.visible = false
