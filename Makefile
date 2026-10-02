.PHONY: check

check:
	python3 scripts/check_repository.py
	python3 games/toronto-dispatch/scripts/check_campaign.py
