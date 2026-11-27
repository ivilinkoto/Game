extends Node3D
## CARGO STATE — прототип.
## Малка карта с пътища и 5 бази. Закачаш ремарке с товар на една база и го караш до друга.

const SAVE_PATH := "user://save.cfg"
const ZONE_RADIUS := 18.0
const ATTACH_DIST := 1.4
const TrailerScript := preload("res://scripts/trailer.gd")
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

var truck: VehicleBody3D
var cam: Camera3D
var hud: CanvasLayer
var touch: CanvasLayer
var arrow: Node3D
var beacon: MeshInstance3D

var money := 500
var deliveries := 0
var trailers: Array[TrailerScript] = []
var offers := {}                     # база -> ремарке с поръчка (или null)
var attached: TrailerScript = null
var joint: Generic6DOFJoint3D = null
var respawn_timer := 0.0


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

	for d in DEPOTS.size():
		_make_offer(d)

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
		_static_box(Vector3(50, 0.06, 50), p + Vector3(0, 0.03, 0), concrete, false)
		# Жълт кант на зоната
		for s in [-1, 1]:
			_static_box(Vector3(36, 0.03, 0.5), p + Vector3(0, 0.07, s * 18), stripe, false)
			_static_box(Vector3(0.5, 0.03, 36), p + Vector3(s * 18, 0.07, 0), stripe, false)
		# Склад зад зоната (от другата страна на пътя)
		var out := Vector3(p.x, 0, p.z).normalized()
		var hall := p + out * 42.0
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


# ---------------------------------------------------------------- ремаркета и поръчки

func _depot_at(pos: Vector3) -> int:
	for i in DEPOTS.size():
		var p: Vector3 = DEPOTS[i]["pos"]
		if Vector2(pos.x - p.x, pos.z - p.z).length() < ZONE_RADIUS:
			return i
	return -1


func _offer_spot(depot: int) -> Vector3:
	var p: Vector3 = DEPOTS[depot]["pos"]
	return p + Vector3(p.x, 0, p.z).normalized() * 8.0


func _make_offer(depot: int) -> void:
	var options := []
	for i in DEPOTS.size():
		if i != depot:
			options.append(i)
	var to: int = options[randi() % options.size()]
	var c := randi() % CARGO.size()
	var dist: float = (DEPOTS[depot]["pos"] as Vector3).distance_to(DEPOTS[to]["pos"])
	var pay := int(round((200.0 + dist * 2.2) * CARGO[c]["rate"] / 10.0)) * 10
	var tr: TrailerScript = TrailerScript.new()
	var p: Vector3 = DEPOTS[depot]["pos"]
	var out := Vector3(p.x, 0, p.z).normalized()
	# Предницата (+Z) гледа към центъра на картата, за да може влекачът да даде назад до нея
	tr.transform = Transform3D(Basis.looking_at(out, Vector3.UP), _offer_spot(depot) + Vector3(0, 1.0, 0))
	add_child(tr)
	tr.setup(c, CARGO[c]["color"], depot, to, pay,
		"%s\n→ %s\n$%d" % [CARGO[c]["name"], DEPOTS[to]["name"], pay])
	trailers.append(tr)
	offers[depot] = tr


func _attach_candidate() -> TrailerScript:
	if attached != null or truck.speed_kmh() > 6.0:
		return null
	var fw: Vector3 = truck.fifth_wheel_global()
	for tr: TrailerScript in trailers:
		if tr.delivered:
			continue
		var kp := tr.kingpin_global()
		if Vector2(fw.x - kp.x, fw.z - kp.z).length() > ATTACH_DIST:
			continue
		if truck.global_transform.basis.z.dot(tr.global_transform.basis.z) < 0.75:
			continue
		return tr
	return null


func _attach(tr: TrailerScript) -> void:
	var fw: Vector3 = truck.fifth_wheel_global()
	var yaw_basis := Basis(Vector3.UP, tr.global_rotation.y)
	tr.global_transform = Transform3D(yaw_basis, fw - yaw_basis * TrailerScript.KINGPIN)
	tr.linear_velocity = truck.linear_velocity
	tr.angular_velocity = Vector3.ZERO

	joint = Generic6DOFJoint3D.new()
	add_child(joint)
	joint.global_transform = Transform3D(Basis(Vector3.UP, truck.global_rotation.y), fw)
	joint.set_flag_y(Generic6DOFJoint3D.FLAG_ENABLE_ANGULAR_LIMIT, false)    # завиване
	joint.set_param_x(Generic6DOFJoint3D.PARAM_ANGULAR_LOWER_LIMIT, -0.2)    # малко накланяне
	joint.set_param_x(Generic6DOFJoint3D.PARAM_ANGULAR_UPPER_LIMIT, 0.2)
	joint.node_a = joint.get_path_to(truck)
	joint.node_b = joint.get_path_to(tr)

	attached = tr
	tr.set_attached(true)
	truck.has_trailer = true
	for d in offers.keys():
		if offers[d] == tr:
			offers[d] = null
	hud.toast("Закачено: %s" % CARGO[tr.cargo]["name"], Color(1, 0.85, 0.4))


func _detach() -> void:
	var tr := attached
	joint.queue_free()
	joint = null
	attached = null
	truck.has_trailer = false
	tr.brake = 30.0
	tr.set_attached(false)
	if not tr.delivered and _depot_at(tr.global_position) == tr.to_depot:
		tr.delivered = true
		tr.set_attached(false)
		money += tr.pay
		deliveries += 1
		hud.toast("Доставено!  + $%d" % tr.pay)
		hud.set_money(money, deliveries)
		_save()
	else:
		hud.toast("Ремаркето е откачено", Color(1, 0.85, 0.4))


func _interact() -> void:
	if attached != null:
		if truck.speed_kmh() < 3.0:
			_detach()
		return
	var tr := _attach_candidate()
	if tr != null:
		_attach(tr)


func _nearest_trailer() -> TrailerScript:
	var best: TrailerScript = null
	var best_d := INF
	for tr: TrailerScript in trailers:
		if tr.delivered:
			continue
		var d := truck.global_position.distance_to(tr.global_position)
		if d < best_d:
			best_d = d
			best = tr
	return best


func _maintain_trailers() -> void:
	# Доставените ремаркета изчезват, когато се отдалечиш
	for tr: TrailerScript in trailers.duplicate():
		if tr.delivered and tr != attached and truck.global_position.distance_to(tr.global_position) > 70.0:
			trailers.erase(tr)
			tr.queue_free()
	# Нова поръчка на всяка база без ремарке (когато не гледаш)
	for d in DEPOTS.size():
		if offers.get(d) != null:
			continue
		var spot := _offer_spot(d)
		if truck.global_position.distance_to(spot) < 90.0:
			continue
		var free := true
		for tr: TrailerScript in trailers:
			if tr.global_position.distance_to(spot) < 16.0:
				free = false
		if free:
			_make_offer(d)


func _update_job_text() -> void:
	if attached != null:
		var dest: Vector3 = DEPOTS[attached.to_depot]["pos"]
		if _depot_at(attached.global_position) == attached.to_depot:
			hud.set_job("Пристигна: %s" % DEPOTS[attached.to_depot]["name"], "Спри и натисни ОТКАЧИ, за да предадеш товара ($%d)" % attached.pay)
		elif attached.delivered:
			hud.set_job("Празно ремарке", "Откачи го, за да вземеш нова поръчка")
		else:
			hud.set_job("%s → %s" % [CARGO[attached.cargo]["name"], DEPOTS[attached.to_depot]["name"]],
				"Остават %.1f км   ·   заплащане $%d" % [truck.global_position.distance_to(dest) / 1000.0, attached.pay])
	elif _attach_candidate() != null:
		hud.set_job("Ремаркето е зад теб", "Натисни ЗАКАЧИ")
	else:
		var tr := _nearest_trailer()
		var where := ""
		if tr != null and _depot_at(tr.global_position) >= 0:
			where = " (най-близо: %s)" % DEPOTS[_depot_at(tr.global_position)]["name"]
		hud.set_job("Няма ремарке", "Иди до ремарке с поръчка%s, обърни и дай назад до него" % where)


func _process(delta: float) -> void:
	hud.set_speed(truck.speed_kmh())
	if attached != null:
		attached.brake = truck.brake
	cam.with_trailer = attached != null
	cam.docking = false
	if attached == null:
		var near := _nearest_trailer()
		if near != null and truck.fifth_wheel_global().distance_to(near.kingpin_global()) < 22.0:
			cam.docking = true

	respawn_timer -= delta
	if respawn_timer <= 0.0:
		respawn_timer = 1.0
		_maintain_trailers()

	var btn := ""
	if attached != null and truck.speed_kmh() < 3.0:
		btn = "ОТКАЧИ"
	elif _attach_candidate() != null:
		btn = "ЗАКАЧИ"
	touch.set_interact(btn)
	_update_job_text()

	# Стрелка над кабината и светъл стълб над целта
	var t := truck.global_position
	if attached != null:
		t = DEPOTS[attached.to_depot]["pos"]
	else:
		var tr := _nearest_trailer()
		if tr != null:
			t = tr.kingpin_global()
	arrow.global_position = truck.global_position + Vector3(0, 4.6, 0)
	var flat := Vector3(t.x, arrow.global_position.y, t.z)
	arrow.visible = flat.distance_to(arrow.global_position) > 8.0
	if arrow.visible:
		arrow.look_at(flat, Vector3.UP)
	beacon.global_position = Vector3(t.x, 30, t.z)
	beacon.visible = truck.global_position.distance_to(t) > 25.0


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
