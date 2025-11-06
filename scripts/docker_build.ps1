$my_path = $PSScriptRoot;

docker build -f $my_path/../src/server/Dockerfile -t py_server:0.1 $my_path/../src/server/