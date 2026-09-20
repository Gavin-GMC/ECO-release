import os
from typing import Any, List, Optional, Sequence, Union

from ._core import (
    ComponentInfo as ComponentInfo,
    Config as Config,
    ConfigFileReport as ConfigFileReport,
    ConfigFlowCheck as ConfigFlowCheck,
    ConfigIssue as ConfigIssue,
    ConfiguredComponent as ConfiguredComponent,
    ParameterInfo as ParameterInfo,
    ParameterKind as ParameterKind,
    ParameterSchema as ParameterSchema,
    RequirementInfo as RequirementInfo,
    Subpopulation as Subpopulation,
    ValidationReport as ValidationReport,
    Workflow as Workflow,
)

PathInput = Union[str, bytes, os.PathLike]
ComponentParameters = Union[Sequence[Any], dict]

def component_categories() -> List[str]: ...
def component_provisions(category: str, tag: str, parameters: ComponentParameters = ...) -> List[RequirementInfo]: ...
def component_requirements(category: str, tag: str, parameters: ComponentParameters = ...) -> List[RequirementInfo]: ...
def list_components(category: Optional[str] = ...) -> List[ComponentInfo]: ...
def list_initializers() -> List[ComponentInfo]: ...
def resolve_parameter_schema(category: str, tag: str, parameters: ComponentParameters = ...) -> ParameterSchema: ...
def validate_config_path(path: PathInput, recursive: bool = ..., deep: bool = ...) -> ValidationReport: ...
def validate_config_text(text: str, deep: bool = ..., file_name: str = ...) -> ValidationReport: ...
