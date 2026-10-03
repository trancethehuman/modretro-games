.PHONY: check

check:
	python3 games/toronto-dispatch/scripts/create_atlas.py --check
	python3 scripts/test_atlas.py
	python3 scripts/test_atlas_ui.py
	python3 scripts/test_atlas_banks.py
	python3 scripts/check_repository.py
	python3 games/toronto-dispatch/scripts/check_campaign.py
	python3 games/toronto-dispatch/scripts/create_campaign.py --check-streets
	python3 games/toronto-dispatch/scripts/check_streetcar.py
	python3 scripts/test_transit.py
	python3 scripts/test_streetcar_motion.py
	python3 games/toronto-dispatch/scripts/create_streetcar_sprite.py --check
	python3 games/toronto-dispatch/scripts/create_aircraft_sprite.py --check
	python3 scripts/test_aircraft.py
	python3 scripts/check_aircraft_rom.py --self-test
	python3 scripts/test_aircraft_render.py
	python3 games/toronto-dispatch/scripts/check_district_world.py
	python3 scripts/test_district_seams.py
	python3 games/toronto-dispatch/scripts/create_district_world.py --check
	python3 games/toronto-dispatch/scripts/create_district_jobs.py --check
	python3 games/toronto-dispatch/scripts/create_east_jobs.py --check
	python3 games/toronto-dispatch/scripts/create_port_jobs.py --check
	python3 games/toronto-dispatch/scripts/create_east_art.py --check
	python3 games/toronto-dispatch/scripts/create_port_lands_art.py --check
	python3 games/toronto-dispatch/scripts/create_world_routes.py --check
	python3 games/toronto-dispatch/scripts/create_audio.py --check
	python3 games/toronto-dispatch/scripts/create_city_sprites.py --check
	python3 games/toronto-dispatch/scripts/create_boat_sprite.py --check
	python3 games/toronto-dispatch/scripts/create_traffic_signals.py --check
	python3 scripts/test_boats.py
	python3 scripts/test_city_sprites.py
	python3 scripts/test_people_hotspots.py
	python3 scripts/test_traffic.py
	python3 scripts/test_roads.py
	python3 scripts/test_police.py
	python3 scripts/test_engine.py
	python3 scripts/test_district_bridge.py
	python3 scripts/test_world_navigation.py
	python3 tests/test_rom_memory.py
