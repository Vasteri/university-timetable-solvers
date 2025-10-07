from fastapi import FastAPI
from pydantic import BaseModel
import torch
import torch.nn as nn

# ----- Модель -----
class SimpleModel(nn.Module):
    def __init__(self):
        super().__init__()
        self.fc = nn.Linear(2, 1)

    def forward(self, x):
        return self.fc(x)

model = SimpleModel()
model.eval()  # режим инференса

# ----- API -----
app = FastAPI()

class InputData(BaseModel):
    x1: float
    x2: float

@app.post("/predict")
def predict(data: InputData):
    with torch.no_grad():
        x = torch.tensor([[data.x1, data.x2]], dtype=torch.float32)
        y = model(x)
        return {"prediction": y.item()}
