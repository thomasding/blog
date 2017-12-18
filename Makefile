.PHONY: deploy

deploy:
	JEKYLL_ENV=production jekyll build --destination _deploy
	rsync -avzP --delete _deploy/ tding@tomatoast.pw:www/
