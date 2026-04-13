$my_path = $PSScriptRoot + "/../.."

uvicorn main:app --app-dir $my_path/src/server/ --reload --host 0.0.0.0