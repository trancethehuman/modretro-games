.PHONY: check

check:
	python3 scripts/create_atlas.py --check
	python3 scripts/test_atlas.py
	python3 scripts/test_atlas_ui.py
	python3 scripts/check_repository.py
	python3 scripts/check_campaign.py
	python3 scripts/check_streetcar.py
	python3 scripts/test_transit.py
	python3 scripts/check_district_world.py
	python3 scripts/create_district_world.py --check
	python3 scripts/create_district_jobs.py --check
	python3 scripts/create_east_jobs.py --check
	python3 scripts/create_city_art.py --check
	python3 scripts/create_west_art.py --check
	python3 scripts/create_east_art.py --check
	python3 scripts/sync_city_resources.py --check
	python3 scripts/create_interiors.py --check
	python3 scripts/create_sprites.py --check
	python3 scripts/create_street_life.py --check
	python3 scripts/create_people.py --check
	python3 scripts/create_ui_art.py --check
	python3 scripts/create_daynight.py --check
	python3 scripts/create_scenery.py --check
	python3 scripts/create_overlay.py --check
	python3 scripts/create_hidden_vehicles.py --check
	python3 scripts/create_radio.py --check
	python3 scripts/check_story.py
	python3 scripts/create_world_routes.py --check
	python3 scripts/create_places.py --check
	python3 scripts/create_audio.py --check
	python3 scripts/test_engine.py
	python3 scripts/test_district_bridge.py
	python3 scripts/test_world_navigation.py
	python3 tests/test_rom_memory.py
