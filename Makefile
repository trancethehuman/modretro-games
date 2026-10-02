.PHONY: check

check:
	python3 scripts/check_repository.py
	python3 games/toronto-dispatch/scripts/check_campaign.py
	python3 games/toronto-dispatch/scripts/create_world_routes.py --check
	python3 games/toronto-dispatch/scripts/create_audio.py --check
	python3 scripts/test_engine.py
