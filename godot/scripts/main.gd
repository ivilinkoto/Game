extends Node3D
## CARGO STATE — прототип.
## Малка карта с пътища и 5 бази. Взимаш товар от една база и го караш до друга.

const SAVE_PATH := "user://save.cfg"
const ZONE_RADIUS := 14.0
const LOAD_TIME := 4.0
const ROAD_W := 12.0

const DEPOTS := [
	{"name": "Склад „Север“", "pos": Vector3(0, 0, -275)},
	{"name": "Завод „Изток“", "pos": Vector3(275, 0, 0)},
	{"name": "Пристанище „Юг“", "pos": Vector3(0, 0, 275)},
	{"name": "Ферма „Запад“", "pos": Vector3(-275, 0, 0)},
	{"name": "Логистичен център", "pos": Vector3(-222, 0, -222)},
]

const CARGO := [
	{"name": "Дървен материал", "rate": 1.0, "color": Color(0.62, 0.45, 0.25)},
	{"name": "Храни", "rate": 1.15, "color": Color(0.9, 0.9, 0.85)},
	{"name": "Стомана", "rate": 1.3, "color": Color(0.55, 0.58, 0.62)},
	{"name": "Електроника", "rate": 1.5, "color": Color(0.2, 0.35, 0.7)},
	{"name": "Строителни материали", "rate": 1.1, "color": Color(0.75, 0.7, 0.6)},
]

enum State { FREE, LOADING, DELIVERING, UNLOADING }

var truck: VehicleBody3D
var cam: Camera3D
var hud: CanvasLayer
var touch: CanvasLayer
var arrow: Node3D
var beacon: MeshInstance3D

var money := 500
var deliveries := 0
var state := State.FREE
var timer := 0.0
var job_from := -1
var job_to := -1
var job_cargo := 0
var job_pay := 0


func _ready() -> void:
	_setup_input()
	_build_environment()
	_build_ground()
	_build_roads()
	_build_depots()
	_build_city()

	truck = preload("res://scripts/truck.gd").new()
	truck.position = Vector3(0, 1.5, -272)
	truck.rotation.y = 0.0
	add_child(truck)

	cam = preload("res://scripts/follow_camera.gd").new()
	cam.target = truck
	cam.position = truck.position + Vector3(0, 6, -14)
	add_child(cam)
	cam.make_current()

	arrow = _make_arrow()
	add_child(arrow)
	beacon = _make_beacon()
	add_child(beacon)

	hud = preload("res://scripts/hud.gd").new()
	add_child(hud)
	touch = preload("res://scripts/touch_controls.gd").new()
	touch.visible = DisplayServer.is_touchscreen_available()
	add_child(touch)

	_load()
	hud.set_money(money, deliveries)
	_update_job_text()


# ---------------------------------------------------------------- управление

func _add_key(action: String, keys: Array) -> void:
	if not InputMap.has_action(action):
		InputMap.add_action(action)
	for k in keys:
		var ev := InputEventKey.new()
		ev.physical_keycode = k
		InputMap.action_add_event(action, ev)


func _setup_input() -> void:
	_add_key("accelerate", [KEY_W, KEY_UP])
	_add_key("brake", [KEY_S, KEY_DOWN])
	_add_key("steer_left", [KEY_A, KEY_LEFT])
	_add_key("steer_right", [KEY_D, KEY_RIGHT])
	_add_key("handbrake", [KEY_SPACE])
	_add_key("interact", [KEY_E, KEY_ENTER])
	_add_key("camera", [KEY_C])
	_add_key("reset_truck", [KEY_R])


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("camera"):
		cam.next_mode()
	elif event.is_action_pressed("reset_truck"):
		truck.reset_upright()
	elif event.is_action_pressed("interact"):
		_interact()


# ---------------------------------------------------------------- светът

func _mat(color: Color, rough := 0.9) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = color
	m.roughness = rough
	return m


func _build_environment() -> void:
	var env := Environment.new()
	env.background_mode = Environment.BG_SKY
	var sky := Sky.new()
	var sm := ProceduralSkyMaterial.new()
	sm.sky_top_color = Color(0.32, 0.55, 0.85)
	sm.sky_horizon_color = Color(0.75, 0.83, 0.9)
	sm.ground_horizon_color = Color(0.6, 0.65, 0.6)
	sm.ground_bottom_color = Color(0.25, 0.3, 0.25)
	sky.sky_material = sm
	env.sky = sky
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.62, 0.66, 0.72)
	env.ambient_light_energy = 0.45
	env.tonemap_mode = Environment.TONE_MAPPER_LINEAR
	env.fog_enabled = true
	env.fog_light_color = Color(0.7, 0.78, 0.86)
	env.fog_density = 0.0008
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)

	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-50, -35, 0)
	sun.light_energy = 0.9
	sun.shadow_enabled = true
	sun.directional_shadow_max_distance = 120.0
	add_child(sun)


func _static_box(size: Vector3, pos: Vector3, mat: Material, collide := true) -> void:
	var mi := MeshInstance3D.new()
	var bm := BoxMesh.new()
	bm.size = size
	mi.mesh = bm
	mi.material_override = mat
	if collide:
		var body := StaticBody3D.new()
		body.position = pos
		var cs := CollisionShape3D.new()
		var shape := BoxShape3D.new()
		shape.size = size
		cs.shape = shape
		body.add_child(cs)
		body.add_child(mi)
		add_child(body)
	else:
		mi.position = pos
		add_child(mi)


func _build_ground() -> void:
	_static_box(Vector3(1400, 2, 1400), Vector3(0, -1, 0), _mat(Color(0.24, 0.38, 0.17)))


func _road(from: Vector2, to: Vector2) -> void:
	var asphalt := _mat(Color(0.13, 0.13, 0.14), 0.95)
	var line := _mat(Color(0.95, 0.95, 0.9))
	var center := (from + to) / 2.0
	var length := from.distance_to(to)
	var along_x := absf(to.x - from.x) > absf(to.y - from.y)
	var size := Vector3(length + ROAD_W, 0.04, ROAD_W) if along_x else Vector3(ROAD_W, 0.04, length + ROAD_W)
	_static_box(size, Vector3(center.x, 0.02, center.y), asphalt, false)
	# Прекъсната осева линия
	var n := int(length / 8.0)
	for i in n:
		var t := (i + 0.5) / n
		var p := from.lerp(to, t)
		var ls := Vector3(3.0, 0.02, 0.25) if along_x else Vector3(0.25, 0.02, 3.0)
		_static_box(ls, Vector3(p.x, 0.05, p.y), line, false)


func _build_roads() -> void:
	# Кръст през центъра и околовръстен път
	_road(Vector2(0, -262), Vector2(0, 262))
	_road(Vector2(-262, 0), Vector2(262, 0))
	_road(Vector2(-200, -200), Vector2(200, -200))
	_road(Vector2(-200, 200), Vector2(200, 200))
	_road(Vector2(-200, -200), Vector2(-200, 200))
	_road(Vector2(200, -200), Vector2(200, 200))


func _build_depots() -> void:
	var concrete := _mat(Color(0.55, 0.55, 0.52))
	var stripe := _mat(Color(0.95, 0.75, 0.15))
	var wall := _mat(Color(0.75, 0.72, 0.66))
	var roof := _mat(Color(0.35, 0.38, 0.42))
	for i in DEPOTS.size():
		var p: Vector3 = DEPOTS[i]["pos"]
		_static_box(Vector3(40, 0.06, 40), p + Vector3(0, 0.03, 0), concrete, false)
		# Жълт кант на зоната
		for s in [-1, 1]:
			_static_box(Vector3(30, 0.03, 0.5), p + Vector3(0, 0.07, s * 15), stripe, false)
			_static_box(Vector3(0.5, 0.03, 30), p + Vector3(s * 15, 0.07, 0), stripe, false)
		# Склад зад зоната (от другата страна на пътя)
		var out := Vector3(p.x, 0, p.z).normalized()
		var hall := p + out * 32.0
		_static_box(Vector3(26, 9, 26), hall + Vector3(0, 4.5, 0), wall)
		_static_box(Vector3(27, 0.6, 27), hall + Vector3(0, 9.3, 0), roof, false)
		var sign := Label3D.new()
		sign.text = DEPOTS[i]["name"]
		sign.font_size = 160
		sign.pixel_size = 0.02
		sign.outline_size = 24
		sign.billboard = BaseMaterial3D.BILLBOARD_ENABLED
		sign.position = hall + Vector3(0, 13, 0)
		sign.modulate = Color(1, 0.85, 0.4)
		add_child(sign)


func _build_city() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 2026
	var palette := [Color(0.8, 0.78, 0.72), Color(0.65, 0.6, 0.55), Color(0.55, 0.62, 0.7),
		Color(0.85, 0.82, 0.78), Color(0.6, 0.45, 0.38)]
	# Сгради в четирите квартала между кръста и околовръстното
	for qx in [-1, 1]:
		for qz in [-1, 1]:
			for gx in 4:
				for gz in 4:
					if rng.randf() < 0.25:
						continue
					var w := rng.randf_range(14, 26)
					var d := rng.randf_range(14, 26)
					var h := rng.randf_range(8, 40)
					var cx: float = qx * (30 + gx * 42 + 10)
					var cz: float = qz * (30 + gz * 42 + 10)
					_static_box(Vector3(w, h, d), Vector3(cx, h / 2.0, cz), _mat(palette[rng.randi() % palette.size()]))
	# Дървета извън околовръстното
	var trunk := _mat(Color(0.35, 0.25, 0.15))
	var leaves := _mat(Color(0.2, 0.42, 0.18))
	for i in 160:
		var pos := Vector3(rng.randf_range(-480, 480), 0, rng.randf_range(-480, 480))
		if absf(pos.x) < 235 and absf(pos.z) < 235:
			continue
		if absf(pos.x) < 30 or absf(pos.z) < 30:
			continue       # да не е на пътищата към базите
		var near_depot := false
		for dp in DEPOTS:
			if pos.distance_to(dp["pos"]) < 70:
				near_depot = true
		if near_depot:
			continue
		_static_box(Vector3(0.8, 4, 0.8), pos + Vector3(0, 2, 0), trunk)
		var crown := MeshInstance3D.new()
		var sp := SphereMesh.new()
		sp.radius = 3.2
		sp.height = 6.0
		crown.mesh = sp
		crown.material_override = leaves
		crown.position = pos + Vector3(0, 6, 0)
		add_child(crown)


func _make_arrow() -> Node3D:
	var pivot := Node3D.new()
	var mi := MeshInstance3D.new()
	var cone := CylinderMesh.new()
	cone.top_radius = 0.0
	cone.bottom_radius = 0.6
	cone.height = 1.8
	mi.mesh = cone
	mi.rotation_degrees = Vector3(-90, 0, 0)    # върхът сочи напред (-Z)
	var m := _mat(Color(1, 0.75, 0.15), 0.4)
	m.emission_enabled = true
	m.emission = Color(1, 0.6, 0.1)
	m.emission_energy_multiplier = 0.6
	mi.material_override = m
	pivot.add_child(mi)
	return pivot


func _make_beacon() -> MeshInstance3D:
	var mi := MeshInstance3D.new()
	var cyl := CylinderMesh.new()
	cyl.top_radius = 2.0
	cyl.bottom_radius = 2.0
	cyl.height = 60.0
	mi.mesh = cyl
	var m := StandardMaterial3D.new()
	m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	m.albedo_color = Color(1, 0.75, 0.15, 0.35)
	m.cull_mode = BaseMaterial3D.CULL_DISABLED
	mi.material_override = m
	return mi


# ---------------------------------------------------------------- поръчки

func _depot_here() -> int:
	for i in DEPOTS.size():
		var p: Vector3 = DEPOTS[i]["pos"]
		var d := Vector2(truck.global_position.x - p.x, truck.global_position.z - p.z).length()
		if d < ZONE_RADIUS:
			return i
	return -1


func _nearest_depot() -> int:
	var best := 0
	var best_d := INF
	for i in DEPOTS.size():
		var d: float = truck.global_position.distance_to(DEPOTS[i]["pos"])
		if d < best_d:
			best_d = d
			best = i
	return best


func _target() -> int:
	if state == State.DELIVERING or state == State.UNLOADING:
		return job_to
	return _nearest_depot()


func _can_interact() -> bool:
	var here := _depot_here()
	if here < 0 or truck.speed_kmh() > 3.0:
		return false
	if state == State.FREE:
		return true
	return state == State.DELIVERING and here == job_to


func _interact() -> void:
	if not _can_interact():
		return
	timer = LOAD_TIME
	if state == State.FREE:
		job_from = _depot_here()
		state = State.LOADING
	else:
		state = State.UNLOADING
	_update_job_text()


func _start_delivery() -> void:
	var options := []
	for i in DEPOTS.size():
		if i != job_from:
			options.append(i)
	job_to = options[randi() % options.size()]
	job_cargo = randi() % CARGO.size()
	var dist: float = (DEPOTS[job_from]["pos"] as Vector3).distance_to(DEPOTS[job_to]["pos"])
	job_pay = int(round((150.0 + dist * 1.6) * CARGO[job_cargo]["rate"] / 10.0)) * 10
	truck.set_cargo(true, CARGO[job_cargo]["color"])
	state = State.DELIVERING
	hud.toast("Натоварено: %s" % CARGO[job_cargo]["name"], Color(1, 0.85, 0.4))


func _finish_delivery() -> void:
	money += job_pay
	deliveries += 1
	truck.set_cargo(false)
	hud.toast("+ $%d" % job_pay)
	hud.set_money(money, deliveries)
	state = State.FREE
	job_to = -1
	_save()


func _update_job_text() -> void:
	match state:
		State.FREE:
			var n: String = DEPOTS[_nearest_depot()]["name"]
			hud.set_job("Няма товар", "Спри в жълтата зона на база (най-близо: %s) и натисни ТОВАР" % n)
		State.LOADING:
			hud.set_job("Товарене…", "Не мърдай, докато се товари")
		State.DELIVERING:
			var dist := truck.global_position.distance_to(DEPOTS[job_to]["pos"])
			hud.set_job("%s → %s" % [CARGO[job_cargo]["name"], DEPOTS[job_to]["name"]],
				"Остават %.1f км   ·   заплащане $%d" % [dist / 1000.0, job_pay])
		State.UNLOADING:
			hud.set_job("Разтоварване…", "Не мърдай, докато се разтоварва")


func _process(delta: float) -> void:
	hud.set_speed(truck.speed_kmh())

	if state == State.LOADING or state == State.UNLOADING:
		if truck.speed_kmh() > 3.0 or _depot_here() < 0:
			state = State.FREE if state == State.LOADING else State.DELIVERING
			hud.toast("Прекъснато — спри в зоната", Color(1, 0.5, 0.4))
			hud.set_progress(-1)
		else:
			timer -= delta
			hud.set_progress(1.0 - timer / LOAD_TIME)
			if timer <= 0.0:
				hud.set_progress(-1)
				if state == State.LOADING:
					_start_delivery()
				else:
					_finish_delivery()

	touch.set_interact_visible(_can_interact() and state != State.LOADING and state != State.UNLOADING)
	_update_job_text()

	# Стрелка над кабината и светъл стълб над целта
	var t: Vector3 = DEPOTS[_target()]["pos"]
	arrow.global_position = truck.global_position + Vector3(0, 4.6, 0)
	var flat := Vector3(t.x, arrow.global_position.y, t.z)
	if flat.distance_to(arrow.global_position) > 1.0:
		arrow.look_at(flat, Vector3.UP)
	beacon.global_position = t + Vector3(0, 30, 0)
	beacon.visible = state == State.DELIVERING or state == State.UNLOADING or _depot_here() < 0


# ---------------------------------------------------------------- запис

func _save() -> void:
	var cfg := ConfigFile.new()
	cfg.set_value("player", "money", money)
	cfg.set_value("player", "deliveries", deliveries)
	cfg.save(SAVE_PATH)


func _load() -> void:
	var cfg := ConfigFile.new()
	if cfg.load(SAVE_PATH) == OK:
		money = cfg.get_value("player", "money", money)
		deliveries = cfg.get_value("player", "deliveries", deliveries)


func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST or what == NOTIFICATION_APPLICATION_PAUSED:
		_save()
