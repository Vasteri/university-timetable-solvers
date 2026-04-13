my_path="$(dirname $0)/../.."

docker build -f "$my_path"/src/server/Dockerfile -t py_server:0.1 "$my_path"/src/server/