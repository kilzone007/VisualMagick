/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%  Copyright 2014-2021 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/
#include "stdafx.h"
#include "resource.h"
#include "ConfigureWizard.h"
#include "CommandLineInfo.h"

IMPLEMENT_DYNAMIC(ConfigureWizard,CPropertySheet)

ConfigureWizard::ConfigureWizard(CWnd* pWndParent)
  : CPropertySheet(IDS_PROPSHT_CAPTION,pWndParent)
{
  AddPage(&_welcomePage);
  AddPage(&_targetPage);
  AddPage(&_systemPage);
  AddPage(&_finishedPage);

  SetWizardMode();
}

ConfigureWizard::~ConfigureWizard()
{
}

wstring ConfigureWizard::binDirectory() const
{
  return(_systemPage.binDirectory());
}

wstring ConfigureWizard::channelMaskDepth() const
{
  if ((visualStudioVersion() >= VisualStudioVersion::VS2022) && (platform() != Platform::X86))
    return(L"64");
  else
    return(L"32");
}

bool ConfigureWizard::enableDpc() const
{
  return(_targetPage.enableDpc());
}

bool ConfigureWizard::excludeDeprecated() const
{
  return(_targetPage.excludeDeprecated());
}

wstring ConfigureWizard::fuzzBinDirectory() const
{
  return(_systemPage.fuzzBinDirectory());
}

bool ConfigureWizard::includeIncompatibleLicense() const
{
  return(_targetPage.includeIncompatibleLicense());
}

bool ConfigureWizard::includeOptional() const
{
  return(_targetPage.includeOptional());
}

bool ConfigureWizard::installedSupport() const
{
	return(_targetPage.installedSupport());
}

wstring ConfigureWizard::libDirectory() const
{
  return(_systemPage.libDirectory());
}

wstring ConfigureWizard::machineName() const
{
  switch (platform())
  {
    case Platform::X86: return(L"X86");
    case Platform::X64: return(L"X64");
    case Platform::ARM64: return(L"ARM64");
    default: throw;
  }
}

Platform ConfigureWizard::platform() const
{
  return(_targetPage.platform());
}

wstring ConfigureWizard::platformName() const
{
  switch (platform())
  {
    case Platform::X86: return(L"Win32");
    case Platform::X64: return(L"x64");
    case Platform::ARM64: return(L"ARM64");
    default: throw;
  }
}

wstring ConfigureWizard::platformAlias() const
{
  switch (platform())
  {
    case Platform::X86: return(L"x86");
    case Platform::X64: return(L"x64");
    case Platform::ARM64: return(L"arm64");
    default: throw;
  }
}

PolicyConfig ConfigureWizard::policyConfig() const
{
  return(_targetPage.policyConfig());
}

QuantumDepth ConfigureWizard::quantumDepth() const
{
  return(_targetPage.quantumDepth());
}

wstring ConfigureWizard::solutionName() const
{
  if (solutionType() == SolutionType::DYNAMIC_MT)
    return(L"DynamicMT");
  else if (solutionType() == SolutionType::STATIC_MTD)
    return(L"StaticMTD");
  else if (solutionType() == SolutionType::STATIC_MT)
    return(L"StaticMT");
  else
    return(L"ThisShouldNeverHappen");
}

SolutionType ConfigureWizard::solutionType() const
{
  return(_targetPage.solutionType());
}

bool ConfigureWizard::useHDRI() const
{
  return(_targetPage.useHDRI());
}

bool ConfigureWizard::useOpenCL() const
{
  return(_targetPage.useOpenCL());
}

bool ConfigureWizard::useOpenMP() const
{
  return(_targetPage.useOpenMP());
}

VisualStudioVersion ConfigureWizard::visualStudioVersion() const
{
  return(_targetPage.visualStudioVersion());
}

wstring ConfigureWizard::visualStudioVersionName() const
{
  switch(_targetPage.visualStudioVersion())
  {
    case VisualStudioVersion::VS2017: return(L"VS2017");
    case VisualStudioVersion::VS2019: return(L"VS2019");
    case VisualStudioVersion::VS2022: return(L"VS2022");
    default: return(L"VS");
  }
}

bool ConfigureWizard::zeroConfigurationSupport() const
{
  return(_targetPage.zeroConfigurationSupport());
}

void ConfigureWizard::parseCommandLineInfo(const CommandLineInfo &info)
{
  _targetPage.platform(info.platform());
  _targetPage.enableDpc(info.enableDpc());
  _targetPage.excludeDeprecated(info.excludeDeprecated());
  _targetPage.includeIncompatibleLicense(info.includeIncompatibleLicense());
  _targetPage.includeOptional(info.includeOptional());
  _targetPage.installedSupport(info.installedSupport());
  _targetPage.policyConfig(info.policyConfig());
  _targetPage.quantumDepth(info.quantumDepth());
  _targetPage.solutionType(info.solutionType());
  _targetPage.useHDRI(info.useHDRI());
  _targetPage.useOpenCL(info.useOpenCL());
  _targetPage.useOpenMP(info.useOpenMP());
  _targetPage.visualStudioVersion(info.visualStudioVersion());
  _targetPage.zeroConfigurationSupport(info.zeroConfigurationSupport());
}

wstring ConfigureWizard::cmakeMinVersion() const
{
  return L"3.18";
}

BEGIN_MESSAGE_MAP(ConfigureWizard,CPropertySheet)
END_MESSAGE_MAP()
