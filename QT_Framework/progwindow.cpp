#include "progwindow.h"
#include "ui_progwindow.h"

using namespace std;

progWindow::progWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::progWindow)
{
    ui->setupUi(this);

    // Internal Table Data Storage Setup
    table.addRows(1); // add 1 row (set) to start program

    // UI Table Setup
    ui->tableWidget->clear(); // clear the table
    updateTable(true, true);

    // Misc. Setup
    ui->numSets->setValue(1);
    ui->startX->setValue(table.startX);
    ui->startY->setValue(table.startY);
    ui->setSpacing->setValue(table.setSpacing);

    //SVG Viewer setup
    ui->SVGViewer->setup();
    updatePreviewWindow();
}

progWindow::~progWindow()
{
    delete ui;
}


void progWindow::log(QString message, enum logType messageType = logType::Standard)
{
    // If current message type is an active log type
    if(std::find(activeLogTypes.begin(), activeLogTypes.end(), messageType) != activeLogTypes.end()) {
        ui->consoleOutput->insertPlainText(message + "\n");
    }
}


void progWindow::updatePreviewWindow()
{
    std::vector<QLineF> lines = table.qLines(); // vector of lines to add to window
    ui->SVGViewer->scene()->clear(); // clear the window
    for(size_t i=0; i < lines.size(); i++){ // for each line
        ui->SVGViewer->scene()->addLine(lines[i], linePen); // add the line to the scene
    }
}


void progWindow::CheckCell(int row, int column)
{
    QString cellText = ui->tableWidget->item(row, column)->text();

    if(cellText == table.data[row][column].toQString()){
        //Log("Value to check is already stored in the table", logType::Debug);
        return; // break if the UI table value is already stored in the internal table
    }

    log(QString::fromStdString("Checking row " + std::to_string(row) +
                               " and column " + std::to_string(column)), logType::Debug);
    log("value checked was \"" + cellText + "\"", logType::Debug);

    errorType error_state = table.data[row][column].updateData(cellText); // Update cell value and get back error code
    switch(error_state){
    case errorNone: // no error
        log("The conversion to int was successful and read as " +
            QString::number(table.data[row][column].value), logType::Debug);
        break;
    case errortooSmall: // number was too small and set to min
        log("The number entered is too small\nThe min value is " +
            QString::number(table.data[row][column].min), logType::Error);
        break;
    case errortooLarge: // number was too large and set to max
        log("The number entered is too large\nThe max value is " +
            QString::number(table.data[row][column].max), logType::Error);
        break;
    case errorCannotConvert: // Could not convert QString to valid type
        log("There was an error with the data. The value was cleared", logType::Error);
        break;
    default:
        break;
    }

    // Update cell text for user interface
    ui->tableWidget->item(row, column)->setText(table.data[row][column].toQString());
}


void progWindow::updateCell(int row, int column)
{
    // add new widget item to empty cells and change text for existing cells
    QTableWidgetItem* item = ui->tableWidget->item(row,column);
    if(!item || item->text().isEmpty()){
        ui->tableWidget->setItem(row,column, new QTableWidgetItem(table.data[row][column].toQString()));
    }else{
        item->setText(table.data[row][column].toQString());
    }
}

void progWindow::updateTable(bool updateVerticalHeaders = false, bool updateHorizontalHeaders = false)
{
    // Set Table Size
    ui->tableWidget->setRowCount(table.data.size());
    ui->tableWidget->setColumnCount(table.data[0].size);

    // Update Vertical Headers if updateVerticalHeaders is set to true
    if(updateVerticalHeaders){
        QStringList verticalHeaders = QStringList(); // Create new string list for setting vertical headers
        for(int i = 0; i < ui->tableWidget->rowCount(); i++){ // For each row
            verticalHeaders.append(QString::fromStdString("Set " + std::to_string(i + 1))); // Add string to list
        }
        ui->tableWidget->setVerticalHeaderLabels(verticalHeaders);
    }

    // Update Horizontal Headers if updateHorizontalHeaders is set to true
    if(updateHorizontalHeaders){
        QStringList horizontalHeaders = QStringList(); // Create new string list for setting vertical headers
        for(int i = 0; i < ui->tableWidget->columnCount(); i++){ // For each row
            horizontalHeaders.append(QString::fromStdString(table.data[0][i].typeName)); // Add string to list
        }
        ui->tableWidget->setHorizontalHeaderLabels(horizontalHeaders);
        ui->tableWidget->horizontalHeader()->setVisible(true);
    }

    // Replace cell text for each cell in UI Table
    for(int r = 0; r < ui->tableWidget->rowCount(); r++){
        for(int c = 0; c < ui->tableWidget->columnCount(); c++){
            updateCell(r,c);
        }
    }
}


/**************************************************************************
 *                                SLOTS                                   *
 **************************************************************************/


void progWindow::on_back2Home_clicked()
{
    this->close();
    emit firstWindow();
}


void progWindow::on_numSets_valueChanged(int rowCount)
{
    int prevRowCount = table.data.size(); // get previous row count
    // set new row count
    if(rowCount > prevRowCount){ // If adding rows
        table.addRows(rowCount - prevRowCount);
        updateTable(true,false);
        updatePreviewWindow();
    }
    if(rowCount < prevRowCount){ // If removing rows
        table.removeRows(prevRowCount - rowCount);
        updateTable(true,false);
        updatePreviewWindow();
    }
}


void progWindow::on_tableWidget_cellChanged(int row, int column)
{
    CheckCell(row, column); // Check the cell that was changed
    updatePreviewWindow();
    ui->consoleOutput->ensureCursorVisible(); // Scroll to new content on console
}


void progWindow::on_startX_valueChanged(double arg1)
{
    table.startX = arg1;
    updatePreviewWindow();
}


void progWindow::on_startY_valueChanged(double arg1)
{
    table.startY = arg1;
    updatePreviewWindow();
}


void progWindow::on_setSpacing_valueChanged(double arg1)
{
    table.setSpacing = arg1;
    updatePreviewWindow();
}


void progWindow::on_printPercentSlider_sliderMoved(int position)
{
    double percent = (double)position / (double)ui->printPercentSlider->maximum();


    std::vector<QLineF> lines = table.qLines(); // vector of lines to add to window
    ui->SVGViewer->scene()->clear(); // clear the window

    int numLinestoShow = lines.size() * percent;

    for(int i=0; i < numLinestoShow; i++){ // for each line
        ui->SVGViewer->scene()->addLine(lines[i], linePen); // add the line to the scene
    }
}


void progWindow::on_clearConsole_clicked()
{
    ui->consoleOutput->clear();
}


void progWindow::on_startPrint_clicked()
{
    for(int i = 0; i < int(table.numRows()); ++i) {
        printLineSet(i);
    }
}

void progWindow::printLineSet(int setNum) {
    //Find starting position for line set
    float x_start = table.startX;
    for(int i = 0; i < setNum; ++i) {
        x_start = x_start + table.data[i].lineLength.value + 5.0; //5.0 BEING THE SPACE BETWEEN LINE SETS
    }
    float y_start = table.startY;

    //calculate line specifics - TODO: CURRENTLY OVERCONSTRAINED

    //GO TO XSTART, YSTART, ZMAX
    progWindow::connectToController();
    if(g) {
        string xHomeString = "PAX=" + to_string(75000 - (x_start*1000));
        e(GCmd(g, xHomeString.c_str()));
        string yHomeString = "PAY=" + to_string(y_start*100);
        e(GCmd(g, xHomeString.c_str()));
        e(GCmd(g, "BG"));
        e(GMotionComplete(g, "X")); // Wait until limit is reached
        e(GMotionComplete(g, "Y"));
    }
    //TODO - BREAKS HERE, AND Y AXIS GOES FURTHER THAN EXPECTED.
}

void progWindow::e(GReturn rc)
 {
   if (rc != G_NO_ERROR)
     throw rc;
 }

void progWindow::connectToController() {
        //TO DO - DISABLE BUTTONS UNTIL TUNED, SEPERATE INITIALIZATION HOMING BUTTONS

         //TODO Maybe Threading or something like that? The gui is unresponsive until connect function is finished.
         e(GOpen(address, &g)); // Establish connection with motion controller
         e(GCmd(g, "SH XYZ")); // Enable X,Y, and Z motors

         // Controller Configuration
         e(GCmd(g, "MO")); // Ensure motors are off for setup

         // X Axis
         e(GCmd(g, "MTX = -1"));    // Set motor type to reversed brushless
         e(GCmd(g, "CEX = 2"));     // Set Encoder to reversed quadrature
         e(GCmd(g, "BMX = 40000")); // Set magnetic pitch of lienar motor
         e(GCmd(g, "AGX = 1"));     // Set amplifier gain
         e(GCmd(g, "AUX = 9"));     // Set current loop (based on inductance of motor)
         e(GCmd(g, "TLX = 3"));     // Set constant torque limit to 3V
         e(GCmd(g, "TKX = 0"));     // Disable peak torque setting for now

         // Y Axis
         e(GCmd(g, "MTY = 1"));     // Set motor type to standard brushless
         e(GCmd(g, "CEY = 0"));     // Set Encoder to reversed quadrature
         e(GCmd(g, "BMY = 2000"));  // Set magnetic pitch of rotary motor
         e(GCmd(g, "AGY = 1"));     // Set amplifier gain
         e(GCmd(g, "AUY = 11"));    // Set current loop (based on inductance of motor)
         e(GCmd(g, "TLY = 6"));     // Set constant torque limit to 6V
         e(GCmd(g, "TKY = 0"));     // Disable peak torque setting for now

         // Z Axis
         e(GCmd(g, "MTZ = -2.5"));  // Set motor type to standard brushless
         e(GCmd(g, "CEZ = 14"));    // Set Encoder to reversed quadrature
         e(GCmd(g, "AGZ = 0"));     // Set amplifier gain
         e(GCmd(g, "AUZ = 9"));     // Set current loop (based on inductance of motor)
         // Note: There might be more settings especially for this axis I might want to add later

         // H Axis (Jetting Axis)
         e(GCmd(g, "MTH = -2"));    // Set jetting axis to be stepper motor with defualt low
         e(GCmd(g, "AGH = 0"));     // Set gain to lowest value
         e(GCmd(g, "LDH = 3"));     // Disable limit sensors for H axis
         e(GCmd(g, "KSH = .25"));   // Minimize filters on step signals
         e(GCmd(g, "ITH = 1"));     // Minimize filters on step signals

         e(GCmd(g, "BN"));          // Save (burn) these settings to the controller just to be safe

         e(GCmd(g, "SH XYZ"));      // Enable X,Y, and Z motors
         e(GCmd(g, "CN= -1"));      // Set correct polarity for all limit switches

         //MainWindow::on_xHome_clicked();
         if(g){ // If connected to controller
             // Home the X-Axis using the central home sensor index pulse
             e(GCmd(g, "ACX=200000"));   // 200 mm/s^2
             e(GCmd(g, "DCX=200000"));   // 200 mm/s^2
             e(GCmd(g, "JGX=-15000"));   // 15 mm/s jog towards rear limit
             e(GCmd(g, "BGX"));          // Start motion towards rear limit sensor
             e(GMotionComplete(g, "X")); // Wait until limit is reached
             e(GCmd(g, "JGX=15000"));    // 15 mm/s jog towards home sensor
             e(GCmd(g, "HVX=500"));      // 0.5 mm/s on second move towards home sensor
             e(GCmd(g, "FIX"));          // Find index command for x axis
             e(GCmd(g, "BGX"));          // Begin motion on X-axis for homing (this will automatically set position to 0 when complete)
             e(GMotionComplete(g, "X")); // Wait until X stage finishes moving
             e(GCmd(g, "DPX=75000"));    //Offset position so "0" is the rear limit (home is at center of stage, or 75,000 encoder counts)
         }

         //MainWindow::on_yHome_clicked();
         if(g){
             e(GCmd(g, "ACY=200000"));   // 200 mm/s^2
             e(GCmd(g, "DCY=200000"));   // 200 mm/s^2
             e(GCmd(g, "JGY=-25000"));   // 15 mm/s jog towards rear limit
             e(GCmd(g, "BGY"));          // Start motion towards rear limit sensor
             e(GMotionComplete(g, "Y")); // Wait until limit is reached
             e(GCmd(g, "ACY=50000")); // 50 mm/s^2
             e(GCmd(g, "DCY=50000")); // 50 mm/s^2
             e(GCmd(g, "SPY=25000")); // 25 mm/s
             e(GCmd(g, "PRY=201500"));  // 201.5 mm
             e(GCmd(g, "BGY"));
             e(GMotionComplete(g, "Y"));
             e(GCmd(g, "DPY=0"));
         }

         //MainWindow::on_zHome_clicked();
         if(g){ // If connected to controller
             // Home the Z-Axis using an offset from the top limit sensor
             e(GCmd(g, "ACZ=757760"));   //Acceleration of C     757760 steps ~ 1 mm
             e(GCmd(g, "DCZ=757760"));   //Deceleration of C     7578 steps ~ 1 micron
             e(GCmd(g, "JGZ=113664"));    // Speed of Z
             try {
                 e(GCmd(g, "BGZ")); // Start motion towards rear limit sensor
             } catch(...) {}
             e(GMotionComplete(g, "Z")); // Wait until limit is reached
             e(GCmd(g, "DPZ=0"));    //Offset position so "0" is the rear limit (home is at center of stage, or 75,000 encoder counts)
         }
}

