my_path="$(dirname $0)/../.."

uvicorn main:app --app-dir $my_path/src/server/ --reload --host 0.0.0.0