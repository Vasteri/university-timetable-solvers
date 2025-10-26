$my_path = $PSScriptRoot

cd $my_path/../src/server
uvicorn main:app