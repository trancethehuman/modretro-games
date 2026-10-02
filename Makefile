.PHONY: check

check:
	python3 scripts/check_repository.py
	python3 games/toronto-dispatch/scripts/check_campaign.py
	python3 scripts/test_engine.py
