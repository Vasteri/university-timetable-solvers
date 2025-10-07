import tkinter as tk
from tkinter import messagebox
import requests

API_URL = "http://127.0.0.1:8000/predict"  # наш API

def predict():
    try:
        x1 = float(entry_x1.get())
        x2 = float(entry_x2.get())
        resp = requests.post(API_URL, json={"x1": x1, "x2": x2})
        resp.raise_for_status()
        data = resp.json()
        result_label.config(text=f"Result: {data['prediction']:.4f}")
    except Exception as e:
        messagebox.showerror("Error", str(e))

root = tk.Tk()
root.title("PyTorch API Client")

tk.Label(root, text="x1:").grid(row=0, column=0, padx=5, pady=5)
entry_x1 = tk.Entry(root)
entry_x1.grid(row=0, column=1, padx=5, pady=5)

tk.Label(root, text="x2:").grid(row=1, column=0, padx=5, pady=5)
entry_x2 = tk.Entry(root)
entry_x2.grid(row=1, column=1, padx=5, pady=5)

predict_button = tk.Button(root, text="Predict", command=predict)
predict_button.grid(row=2, column=0, columnspan=2, pady=10)

result_label = tk.Label(root, text="Result: ")
result_label.grid(row=3, column=0, columnspan=2, pady=5)

root.mainloop()
