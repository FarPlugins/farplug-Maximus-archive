(*******************************************************************************
                        P R I M A R Y     L I C E N S E
********************************************************************************

BESEN is copyrighted free software by Benjamin Rosseaux <benjamin@rosseaux.com>.
You can redistribute it and/or modify it under either the terms of the AGPLv3
(see COPYING.txt file), or the conditions below:

  1. You may make and give away verbatim copies of the source form of this
     software without restriction, provided that you duplicate all of the
     original copyright notices and associated disclaimers.

  2. You may modify your copy of this software in any way, provided that
     you do at least ONE of the following:

       a) place your modifications in the Public Domain or otherwise
          make them freely available, such as by posting said
	        modifications to Usenet or an equivalent medium, or by allowing
	        the author to include your modifications in this software.

       b) use the modified software only within your corporation or
          organization.

       c) make other distribution arrangements with the author.

  3. You may distribute this software in object code or executable
     form, provided that you do at least ONE of the following:

       a) distribute the executables and library files of this software,
	        together with instructions (in the manual page or equivalent)
	        on where to get the original distribution.

       b) accompany the distribution with the machine-readable source of
      	  this software.

       c) make other distribution arrangements with the author.

  4. You are permitted to link this software with other software, to embed this
     software in your own software, or to build stand-alone binary or bytecode
     versions of applications that include this software, provided that you do
     at least ONE of the following:

       a) place the other software, if it is our own, in the Public Domain
          or otherwise make them together with machine-readable sources
          freely available, such as by posting said modifications to
          Usenet or an equivalent medium.

       b) use the other software, which includes this software, only within
          your corporation or organization and don't make it accessible or
          distribute it to the public / 3rd parties.

       c) make other distribution arrangements with the author.

  5. The scripts and library files supplied as input to or produced as
     output from the software do not automatically fall under the
     copyright of the software, but belong to whomever generated them,
     and may be sold commercially, and may be aggregated with this
     software.

  6. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
     OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
     AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
     OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
     THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
     DAMAGE.

********************************************************************************
                        S E C O N D A R Y    L I C E N S E
********************************************************************************

    BESEN - A ECMAScript Fifth Edition Object Pascal Implementation
    Copyright (C) 2009-2010  Benjamin 'BeRo' Rosseaux

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be usefufl,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************)
unit BESENObjectBindingFunction;
{$i BESEN.inc}

interface

uses BESENConstants,BESENTypes,BESENObject,BESENObjectFunction,BESENValue,BESENObjectPropertyDescriptor,BESENEnvironmentRecord;

type TBESENObjectBindingFunction=class(TBESENObjectFunction)
      public
       TargetFunction:TBESENObject;
       BoundThis:TBESENValue;
       BoundArguments:TBESENValues;
       constructor Create(AInstance:TObject;APrototype:TBESENObject=nil;AHasPrototypeProperty:longbool=false); overload; override;
       destructor Destroy; override;
       function GetEx(const P:TBESENString;var AResult:TBESENValue;var Descriptor:TBESENObjectPropertyDescriptor;Base:TBESENObject=nil;Hash:TBESENHash=0):boolean; override;
       function GetIndex(const Index,ID:integer;var AResult:TBESENValue;Base:TBESENObject=nil):boolean; override;
       procedure Construct(const ThisArgument:TBESENValue;Arguments:PPBESENValues;CountArguments:integer;var AResult:TBESENValue); override;
       procedure Call(const ThisArgument:TBESENValue;Arguments:PPBESENValues;CountArguments:integer;var AResult:TBESENValue); override;
       function HasInstance(const AInstance:TBESENValue):TBESENBoolean; override;
       function HasConstruct:TBESENBoolean; override;
       function HasCall:TBESENBoolean; override;
       function HasHasInstance:TBESENBoolean; override;
       procedure Finalize; override;
       procedure Mark; override;
     end;

implementation

uses BESEN,BESENErrors,BESENHashUtils;

constructor TBESENObjectBindingFunction.Create(AInstance:TObject;APrototype:TBESENObject=nil;AHasPrototypeProperty:longbool=false);
begin
 inherited Create(AInstance,APrototype,AHasPrototypeProperty);
 ObjectClassName:='Function';
 ObjectName:='Binding';
 TargetFunction:=nil;
 BoundThis:=BESENUndefinedValue;
 BoundArguments:=nil;
end;

destructor TBESENObjectBindingFunction.Destroy;
begin
 TargetFunction:=nil;
 BoundThis:=BESENUndefinedValue;
 SetLength(BoundArguments,0);
 inherited Destroy;
end;

function TBESENObjectBindingFunction.GetEx(const P:TBESENString;var AResult:TBESENValue;var Descriptor:TBESENObjectPropertyDescriptor;Base:TBESENObject=nil;Hash:TBESENHash=0):boolean;
begin
 if not assigned(Base) then begin
  Base:=self;
 end;
 result:=inherited GetEx(p,AResult,Descriptor,Base,Hash);
end;

function TBESENObjectBindingFunction.GetIndex(const Index,ID:integer;var AResult:TBESENValue;Base:TBESENObject=nil):boolean;
begin
 if not assigned(Base) then begin
  Base:=self;
 end;
 result:=inherited GetIndex(Index,ID,AResult,Base);
end;

procedure TBESENObjectBindingFunction.Construct(const ThisArgument:TBESENValue;Arguments:PPBESENValues;CountArguments:integer;var AResult:TBESENValue);
var CallArgs:TBESENValuePointers;
    i,j:integer;
begin
 CallArgs:=nil;
 if assigned(TargetFunction) and TargetFunction.HasConstruct then begin
  SetLength(CallArgs,length(BoundArguments)+CountArguments);
  try
   j:=0;
   for i:=0 to length(BoundArguments)-1 do begin
    CallArgs[j]:=@BoundArguments[i];
    inc(j);
   end;
   for i:=0 to CountArguments-1 do begin
    CallArgs[j]:=Arguments^[i];
    inc(j);
   end;
   TargetFunction.Construct(BoundThis,@CallArgs[0],length(CallArgs),AResult);
  finally
   SetLength(CallArgs,0);
  end;
 end else begin
  raise EBESENTypeError.Create('No hasConstruct');
 end;
end;

procedure TBESENObjectBindingFunction.Call(const ThisArgument:TBESENValue;Arguments:PPBESENValues;CountArguments:integer;var AResult:TBESENValue);
var CallArgs:TBESENValuePointers;
    i,j:integer;
begin
 CallArgs:=nil;
 if assigned(TargetFunction) and TargetFunction.HasCall then begin
  SetLength(CallArgs,length(BoundArguments)+CountArguments);
  try
   j:=0;
   for i:=0 to length(BoundArguments)-1 do begin
    CallArgs[j]:=@BoundArguments[i];
    inc(j);
   end;
   for i:=0 to CountArguments-1 do begin
    CallArgs[j]:=Arguments^[i];
    inc(j);
   end;
   TBESEN(Instance).ObjectCall(TargetFunction,BoundThis,@CallArgs[0],length(CallArgs),AResult);
  finally
   SetLength(CallArgs,0);
  end;
 end else begin
  AResult.ValueType:=bvtUNDEFINED;
 end;
end;

function TBESENObjectBindingFunction.HasInstance(const AInstance:TBESENValue):TBESENBoolean;
begin
 if not TargetFunction.HasHasInstance then begin
  raise EBESENTypeError.Create('No hasInstance');
 end;
 result:=assigned(TargetFunction) and TargetFunction.HasInstance(AInstance);
end;

function TBESENObjectBindingFunction.HasConstruct:TBESENBoolean;
begin
 result:=assigned(TargetFunction) and TargetFunction.HasConstruct;
end;

function TBESENObjectBindingFunction.HasCall:TBESENBoolean;
begin
 result:=assigned(TargetFunction) and TargetFunction.HasCall;
end;

function TBESENObjectBindingFunction.HasHasInstance:TBESENBoolean;
begin
 result:=assigned(TargetFunction) and TargetFunction.HasHasInstance;
end;

procedure TBESENObjectBindingFunction.Finalize;
begin
 TargetFunction:=nil;
 BoundThis:=BESENUndefinedValue;
 SetLength(BoundArguments,0);
 inherited Finalize;
end;

procedure TBESENObjectBindingFunction.Mark;
var i:integer;
begin
 if assigned(TargetFunction) then begin
  TBESEN(Instance).GarbageCollector.GrayIt(TargetFunction);
 end;
 TBESEN(Instance).GarbageCollector.GrayValue(BoundThis);
 for i:=0 to length(BoundArguments)-1 do begin
  TBESEN(Instance).GarbageCollector.GrayValue(BoundArguments[i]);
 end;
 inherited Mark;
end;

end.
