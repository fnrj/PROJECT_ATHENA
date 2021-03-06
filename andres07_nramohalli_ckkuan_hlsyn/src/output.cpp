/*
* File: output.cpp
* Author: Andres Rebeil
* NetID: andres07
* Date: December 1st, 2015
*
* Description: This file .cpp file contains the implementation of the input class
*/
/**************************************************************************************************/
#include "output.h"
/**************************************************************************************************/
//Default Constructor
Output::Output(){
	verilogText = "";
    // nothing
}
// Custom Constructor
Output::Output(char *outputFile, vector<Net>*  netListVector, vector<Node>* nodeListVector){
		this->verilogText = "";
		this->outputFile    = outputFile;
        this->netListVector = netListVector;
        this->nodeListVector= nodeListVector;
}
/**************************************************************************************************/
bool Output::dumpVerilogText(){
    ofstream outputFS;
    outputFS.open(outputFile);
	
    if (!outputFS.is_open()) //Check that valid input file was provided
	{
		cout << "Could not open output file " << outputFile << endl;
		return false;
	}
	else
	{
		outputFS << verilogText;
		outputFS.close();
		return true;
	}
}
/**************************************************************************************************/
bool Output::preMakeProcess(){
    weHaveRegister = false;
    for(unsigned int i=0; i<nodeListVector->size(); i++){
        // Fix comparator output width
        if(nodeListVector->at(i).op=="COMP_LT"||
           nodeListVector->at(i).op=="COMP_EQ"||
           nodeListVector->at(i).op=="COMP_GT")
            nodeListVector->at(i).output->width=1;
        else if(nodeListVector->at(i).op=="REG")
            weHaveRegister = true;
    }
return true;
}
/**************************************************************************************************/
bool Output::makeHead(){
    string thisModule(outputFile);
    thisModule = thisModule.substr(0, thisModule.size()-2); // remove .v extention
    verilogText  =  "`timescale 1ns/1ps\n";
    verilogText +=  "//Author: Generated by hlsyn\n";
    //verilogText +=  "module "+thisModule+"(";
	verilogText += "module HLSM (";
    // loop over inputs
    for(unsigned int i=0; i<netListVector->size(); i++){
        if(netListVector->at(i).type=="input"){
            verilogText += netListVector->at(i).name+", "; 
        }
        else if(netListVector->at(i).type=="output"){
            verilogText += netListVector->at(i).name+", "; 
        }
    }
	verilogText += "Clk, Rst, Start, Done);\n";
	/*
    if(weHaveRegister){
    verilogText += "Clk, Rst, Start, Done ";
    }
    // fix ending
    verilogText.replace(verilogText.end()-2,verilogText.end(),");\n");
	*/
return true;
}
/**************************************************************************************************/
string Output::getNetMatched(unsigned int i, string io, int numOfNets){
   string result;
   int netWidth;
   int nodeWidth = nodeListVector->at(i).width;
   bool netIsSigned;
   bool nodeIsSigned = nodeListVector->at(i).signedBit;
    if(io=="inputs"){
        result      = nodeListVector->at(i).inputs.at(numOfNets)->name;
        netWidth    = nodeListVector->at(i).inputs.at(numOfNets)->width;
        netIsSigned   = nodeListVector->at(i).inputs.at(numOfNets)->signedBit;
    }
    else {
        result = nodeListVector->at(i).output->name;
        netWidth    = nodeListVector->at(i).output->width;
        netIsSigned   = nodeListVector->at(i).output->signedBit;
    }
    if(nodeWidth>netWidth){
        if(netIsSigned&&nodeIsSigned&&(netWidth>1)) //net signed, node signed
            result = result+"["+to_string(netWidth-1)+"]?{{"+to_string(nodeWidth-netWidth)+"{1'd1}},"+result+"}:{{"+to_string(nodeWidth-netWidth)+"{1'd0}},"+result+"}";
        else    // Include three cases:
                //  1. net unsigned, node signed --> postive number so add zero on MSB
                //  2. net signed, node unsigned (not gonna happen, node was only assigned to unsigned if all IOs are unsigned.) 
                //  3. net unsigned, node unsigned (possible), unsgined expanding so pad 0 on MSB 
            result = "{"+to_string(nodeWidth-netWidth)+"'d0,"+result+"}";
    }
    else if(netWidth>nodeWidth){
        if(!netIsSigned&&nodeIsSigned&&(nodeWidth>1))  //net unsigned, node signed
            result = "{1'd0,"+result+"["+to_string(nodeWidth-2)+":0]}";
        else    
            result = result+"["+to_string(nodeWidth-1)+":0]";
    }
    else {// width is match
         if(!netIsSigned&&nodeIsSigned&&(nodeWidth>1))  //net unsigned, node signed
            result = "{1'd0,"+result+"["+to_string(nodeWidth-2)+":0]}";
         else
            {} // do nothing
    }
return result;
}
/**************************************************************************************************/
bool Output::makeNets(){
    // loop over inputs
    for(unsigned int i=0; i<netListVector->size(); i++){
        verilogText += "\t"; 
        if(netListVector->at(i).type=="input"){
            verilogText += "input \t"; 
        }
        else if(netListVector->at(i).type=="output"){
            verilogText += "output \treg ";  //Updated for assignment 3, added reg
        }
        else if(netListVector->at(i).type=="wire"){
            verilogText += "wire \t"; 
        }
        else if(netListVector->at(i).type=="register"){
            verilogText += "reg \t";	//Updated for assignment 3, was wire
        }
        if(netListVector->at(i).signedBit==true){
            verilogText += "signed\t"; 
        }
        verilogText += "["+to_string(netListVector->at(i).width-1)+":0]\t";
        verilogText += netListVector->at(i).name+";";
        verilogText += "\n"; 
    }

    verilogText += "\tinput \tClk, Rst, Start;\n";

return true;
}
/**************************************************************************************************/
bool Output::makeNodes(){
    for(unsigned int i=0; i<nodeListVector->size(); i++){
        verilogText += "\t"; 
        if(nodeListVector->at(i).op!="WIRE"){
            verilogText += nodeListVector->at(i).moduleName+" #(.DATAWIDTH("+to_string(nodeListVector->at(i).width)+"))\t"+nodeListVector->at(i).op+to_string(i)+"(";
        }
        //Mux: reorder the input sequence
        if(nodeListVector->at(i).op=="MUX2x1"){
            verilogText += ".sel("+nodeListVector->at(i).inputs.at(0)->name;
            verilogText += "), .a("+getNetMatched(i,"inputs",1);
            verilogText += "), .b("+getNetMatched(i,"inputs",2);
            verilogText += "), .d("+getNetMatched(i,"output",0);
            verilogText += "));\n";
        }
        else if(nodeListVector->at(i).op=="COMP_EQ"){
            verilogText += ".a("+getNetMatched(i,"inputs",0);
            verilogText += "), .b("+getNetMatched(i,"inputs",1);
            verilogText += "), .eq("+nodeListVector->at(i).output->name;
            verilogText += "));\n";               
        }
        else if(nodeListVector->at(i).op=="COMP_LT"){
            verilogText += ".a("+getNetMatched(i,"inputs",0);
            verilogText += "), .b("+getNetMatched(i,"inputs",1);
            verilogText += "), .lt("+nodeListVector->at(i).output->name;
            verilogText += "));\n";                     
        }
        else if(nodeListVector->at(i).op=="COMP_GT"){
            verilogText += ".a("+getNetMatched(i,"inputs",0);
            verilogText += "), .b("+getNetMatched(i,"inputs",1);
            verilogText += "), .gt("+nodeListVector->at(i).output->name;
            verilogText += "));\n";                     
        }
        else if(nodeListVector->at(i).op=="REG"){
            verilogText += "Clk, Rst, "+getNetMatched(i,"inputs",0)+", "+getNetMatched(i,"output",0)+");\n";
                
        }
        else if(nodeListVector->at(i).op=="WIRE"){
            verilogText += "assign "+getNetMatched(i,"output",0)+" = "+getNetMatched(i,"inputs",0)+";\n";
        }
        else {
        //inputs
            for(unsigned int j=0; j<nodeListVector->at(i).inputs.size(); j++){
                verilogText += getNetMatched(i,"inputs",j) +", ";
            }
        //output
            verilogText += nodeListVector->at(i).output->name +");\n ";
        }
    }
return true;
}
/**************************************************************************************************/
bool Output::makeEnd(){
            verilogText += "endmodule";
return true;
}
/**************************************************************************************************/
bool Output::makeVerilog(){
    preMakeProcess();
    makeHead();
    makeNets();
    makeNodes();
    makeEnd();
    dumpVerilogText();
return true;
}
/**************************************************************************************************/
bool Output::generateHLSM(vector<State>* states){
	//preMakeProcess();
	makeHead();
	makeNets();
	//makeNodes();
	
	makeStateRegParameters(states);
	if (!makeStateMachine(states)) return false;
	makeEnd();
	dumpVerilogText();
	return true;
}
/**************************************************************************************************/
bool Output::makeStateRegParameters(vector<State>* states){
	
	int regSize = 0;
	int totalStates = states->size() + 2;
	//Determine minimum State Register size in bits
	regSize = (int) ceil( log2(totalStates) );

	verilogText += "\n\toutput \treg Done;\n";
	verilogText += "\treg [" + to_string(regSize - 1) + ": 0] State, NextState;\n\n";

	if (totalStates > 2) {
		verilogText += "\tlocalparam [" + to_string(regSize - 1) + ":0] Wait = 0,\n";
		
		for (unsigned int i = 0; i < states->size(); i++)
			verilogText += "\t\t\t\t\t " + states->at(i).name + " = " + to_string(states->at(i).cycle) + ",\n";
		
		verilogText += "\t\t\t\t\t Final = " + to_string(totalStates - 1) + ";\n";
		
		return true;
	}
	return false;
}
/**************************************************************************************************/
bool Output::makeStateMachine(vector<State>* states){

	if (states->size() > 0) {
		this->verilogText += "\talways @(State, Start) begin\n"; //Start Always block
		this->verilogText += "\t\tcase(State)\n";
		this->verilogText += "\t\t\tWait:begin\n"; //Initial Wait state
		this->verilogText += "\t\t\t\tDone <= 0;\n";
		//Reset All Registers
		for (unsigned int i = 0; i < netListVector->size(); i++) {
			if(netListVector->at(i).type == "register" || netListVector->at(i).type == "output")
				this->verilogText += "\t\t\t\t" + netListVector->at(i).name + " <= 0;\n";
		}
		this->verilogText += "\t\t\t\tif(Start)\n";
		if(states->size() > 0)
			this->verilogText += "\t\t\t\t\tNextState <= " + states->at(0).name + ";\n";
		else
			this->verilogText += "\t\t\t\t\tNextState <= Final;\n";
		this->verilogText += "\t\t\t\telse\n";
		this->verilogText += "\t\t\t\t\tNextState <= Wait;\n";
		this->verilogText += "\t\t\tend\n";

		//MAGIC HAPPENS HERE: Add all states generated to verilog output
		for (unsigned int i = 0; i < states->size(); i++) {
			this->verilogText += states->at(i).verilogString; //Generate State i + 1
		}
		//Final State
		this->verilogText += "\t\t\tFinal:begin\n"; //Final Case
		this->verilogText += "\t\t\t\tDone <= 1;\n";
		this->verilogText += "\t\t\t\tNextState <= Wait;\n";
		this->verilogText += "\t\t\tend\n";

		this->verilogText += "\t\t\tdefault:begin\n"; //Default Case
		this->verilogText += "\t\t\t\tNextState <= Wait;\n";
		this->verilogText += "\t\t\tend\n";
		this->verilogText += "\t\tendcase\n";
		this->verilogText += "\tend\n"; //End Always block

		this->verilogText += "\talways @(posedge Clk) begin\n";
		this->verilogText += "\t\tif(Rst)\n";
		this->verilogText += "\t\t\tState <= Wait;\n";
		this->verilogText += "\t\telse\n";
		this->verilogText += "\t\t\tState <=  NextState;\n";
		this->verilogText += "\tend\n";

		return true;
	}
	
	return false;
}

