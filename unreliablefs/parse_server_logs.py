import time
with open("serverevents", "r") as input_file:
    file_data = input_file.readlines()
    data_file = open("data.csv", "w")
    data_file.write("A_proc_time,B_proc_time,First Arrival,Winner\n")
    with open("summary", "w") as summary_file:
        i = 0
        while i < len(file_data):
            lines = []
            lines.append(file_data[i].split())
            i += 1
            lines.append(file_data[i].split())
            i += 1
            lines.append(file_data[i].split())
            i += 1
            lines.append(file_data[i].split())
            i += 1
            arrivals = []
            closes = []
            idx_A = []
            idx_B = []
            for j in range(len(lines)):
                if lines[j][1] == "1":
                    arrivals.append(lines[j])
                else:
                    closes.append(lines[j])
            
            for j in range(len(arrivals)):
                if arrivals[j][2] == "A":
                        idx_A.append(j)
            for j in range(len(closes)):
                if closes[j][2] == "A":
                        idx_A.append(j)
            for j in range(len(arrivals)):
                if arrivals[j][2] == "B":
                        idx_B.append(j)
            for j in range(len(closes)):
                if closes[j][2] == "B":
                        idx_B.append(j)

            A_proc_time = float(closes[idx_A[1]][0]) - float(arrivals[idx_A[0]][0])
            B_proc_time = float(closes[idx_B[1]][0]) - float(arrivals[idx_B[0]][0])
            first_arrival = arrivals[0][2]
            winner = closes[1][2]
            data_file.write(str(A_proc_time) + "," + str(B_proc_time) + "," + first_arrival + "," + winner + "\n")

            line_out = "client " + arrivals[0][2] + " arrived first at time " + arrivals[0][0] + "\n"
            line_out += "client A completed at time " + closes[idx_A[1]][0] + "\n"
            line_out += "client A processing time " +  str(A_proc_time) + "\n"
            line_out += "client B completed at time " + closes[idx_B[1]][0] + "\n"
            line_out += "client B processing time " +  str(B_proc_time) + "\n"
            line_out += "client " + winner + " wins!\n"

            summary_file.write(line_out)
    data_file.close()