$my_path = $PSScriptRoot

uvicorn main:app --app-dir $my_path/../src/server/ --reload