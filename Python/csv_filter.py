import csv, os
import tkinter as tk
from tkinter import filedialog
from tkcalendar import DateEntry

selected_file = None

def filter_csv():
    date = date_entry.get_date().strftime("%Y-%m-%d")
    file_path = filedialog.asksaveasfilename(defaultextension=".csv", initialfile=f"{date}", filetypes=[('CSV files', '*.csv')])

    # read original CSV file
    with open(selected_file, 'r') as file:
        csv_reader = csv.reader(file)
        header = next(csv_reader) # save header row

        # write new CSV file
        with open(file_path, 'w', newline='') as new_file:
            csv_writer = csv.writer(new_file)
            csv_writer.writerow(header) # write header row

            for row in csv_reader:
                if row[1].startswith(date): # check if date matches
                    # get time from row
                    time = row[1][11:19]
                    if time >= '06:00:00' and time <= '22:00:00': # check if time is within range
                        csv_writer.writerow(row) # write row to new file
                elif row[1][:10] > date: # stop reading if past last row with filter date
                    break

    # change filter button text to "Done"
    filter_button.config(text="Done")

def select_file():
    global selected_file
    selected_file = filedialog.askopenfilename()

    # update button text to selected file name
    filename = os.path.basename(selected_file)
    select_button.config(text=filename)

# create UI
root = tk.Tk()
root.title("CSV Filter")
root.geometry("300x120")

# create frame to contain UI elements
frame = tk.Frame(root, width=300, height=120)
frame.place(relx=0.5, rely=0.5, anchor=tk.CENTER)

# create file selection button
select_button = tk.Button(frame, text="Select File", command=select_file)
select_button.grid(row=1, column=0, padx=5, pady=5, sticky="nsew")

# create date input field
date_label = tk.Label(frame, text="Select date:")
date_label.grid(row=0, column=1, padx=5, pady=0, sticky="nsew")

date_entry = DateEntry(frame, date_pattern="yyyy/mm/dd")
date_entry.grid(row=1, column=1, padx=5, pady=5, sticky="e")

# create filter button
filter_button = tk.Button(frame, text="Filter", command=filter_csv)
filter_button.grid(row=1, column=2, padx=5, pady=5, sticky="nsew")

root.mainloop()

