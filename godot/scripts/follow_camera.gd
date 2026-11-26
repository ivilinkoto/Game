extends Camera3D
## Камера зад камиона. Бутонът „Камера" (C) сменя разстоянието.

var target: Node3D
var mode := 0
const MODES := [Vector2(13.0, 5.0), Vector2(20.0, 8.5), Vector2(7.5, 3.2)]  # разстояние, височина


func _ready() -> void:
	fov = 65.0
	far = 1500.0


func next_mode() -> void:
	mode = (mode + 1) % MODES.size()


func _physics_process(delta: float) -> void:
	if target == null:
		return
	var cfg: Vector2 = MODES[mode]
	var back := target.global_transform.basis.z
	back.y = 0.0
	if back.length() < 0.01:
		back = Vector3.FORWARD
	back = back.normalized()
	var want := target.global_position - back * cfg.x + Vector3.UP * cfg.y
	global_position = global_position.lerp(want, clampf(delta * 4.0, 0.0, 1.0))
	look_at(target.global_position + Vector3.UP * 2.0 + back * 4.0, Vector3.UP)
