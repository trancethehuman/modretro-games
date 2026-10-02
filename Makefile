.PHONY: check

check:
	python3 scripts/check_repository.py
	python3 games/toronto-dispatch/scripts/check_campaign.py
	python3 games/toronto-dispatch/scripts/check_district_world.py
	python3 games/toronto-dispatch/scripts/create_district_world.py --check
	python3 games/toronto-dispatch/scripts/create_district_jobs.py --check
	python3 games/toronto-dispatch/scripts/create_east_jobs.py --check
	python3 games/toronto-dispatch/scripts/create_east_art.py --check
	python3 games/toronto-dispatch/scripts/create_world_routes.py --check
	python3 games/toronto-dispatch/scripts/create_audio.py --check
	python3 scripts/test_engine.py
	python3 scripts/test_district_bridge.py
	python3 scripts/test_world_navigation.py
	python3 tests/test_rom_memory.py
