#include "list_controller.h"
#include "../app.h"
#include <assert.h>

using namespace Shared;
using namespace Poincare;

static inline KDCoordinate maxCoordinate(KDCoordinate x, KDCoordinate y) { return x > y ? x : y; }

namespace Sequence {

ListController::ListController(Responder * parentResponder, ::InputEventHandlerDelegate * inputEventHandlerDelegate, ButtonRowController * header, ButtonRowController * footer) :
  Shared::StorageFunctionListController(parentResponder, header, footer, I18n::Message::AddSequence),
  m_sequenceTitleCells{},
  m_expressionCells{},
  m_parameterController(inputEventHandlerDelegate, this),
  m_typeParameterController(this, this, TableCell::Layout::Vertical),
  m_typeStackController(nullptr, &m_typeParameterController, KDColorWhite, Palette::PurpleDark, Palette::PurpleDark),
  m_sequenceToolbox()
{
  for (int i = 0; i < k_maxNumberOfRows; i++) {
    m_expressionCells[i].setLeftMargin(k_expressionMargin);
  }
}

const char * ListController::title() {
  return I18n::translate(I18n::Message::SequenceTab);
}

int ListController::numberOfExpressionRows() {
  int numberOfRows = 0;
  for (int i = 0; i < modelStore()->numberOfModels(); i++) {
    Sequence * sequence = modelStore()->modelForRecord(modelStore()->recordAtIndex(i));
    numberOfRows += sequence->numberOfElements();
  }
  if (modelStore()->numberOfModels() == modelStore()->maxNumberOfModels()) {
    return numberOfRows;
  }
  return 1 + numberOfRows;
};

KDCoordinate ListController::expressionRowHeight(int j) {
  KDCoordinate defaultHeight = Metric::StoreRowHeight;
  if (modelStore()->numberOfModels() < modelStore()->maxNumberOfModels() && j == numberOfRows() - 1) {
    // Add sequence row
    return defaultHeight;
  }
  Sequence * sequence = modelStore()->modelForRecord(modelStore()->recordAtIndex(modelIndexForRow(j)));
  Layout layout = sequence->layout();
  if (sequenceDefinitionForRow(j) == 1) {
    layout = sequence->firstInitialConditionLayout();
  }
  if (sequenceDefinitionForRow(j) == 2) {
    layout = sequence->secondInitialConditionLayout();
  }
  if (layout.isUninitialized()) {
    return defaultHeight;
  }
  KDCoordinate sequenceHeight = layout.layoutSize().height();
  return maxCoordinate(defaultHeight, sequenceHeight + 2*k_expressionCellVerticalMargin);
}

void ListController::willDisplayCellAtLocation(HighlightCell * cell, int i, int j) {
  Shared::StorageFunctionListController::willDisplayCellAtLocation(cell, i, j);
  EvenOddCell * myCell = (EvenOddCell *)cell;
  myCell->setEven(modelIndexForRow(j)%2 == 0);
}

Toolbox * ListController::toolboxForInputEventHandler(InputEventHandler * textInput) {
  // Set extra cells
  int recurrenceDepth = -1;
  int sequenceDefinition = sequenceDefinitionForRow(selectedRow());
  Sequence * sequence = modelStore()->modelForRecord(modelStore()->recordAtIndex(modelIndexForRow(selectedRow())));
  if (sequenceDefinition == 0) {
    recurrenceDepth = sequence->numberOfElements()-1;
  }
  m_sequenceToolbox.buildExtraCellsLayouts(sequence->fullName(), recurrenceDepth);
  // Set sender
  m_sequenceToolbox.setSender(textInput);
  return &m_sequenceToolbox;
}

void ListController::selectPreviousNewSequenceCell() {
  if (sequenceDefinitionForRow(selectedRow()) >= 0) {
    selectCellAtLocation(selectedColumn(), selectedRow()-sequenceDefinitionForRow(selectedRow()));
  }
}

void ListController::editExpression(int sequenceDefinition, Ion::Events::Event event) {
  Ion::Storage::Record record = modelStore()->recordAtIndex(modelIndexForRow(selectedRow()));
  Sequence * sequence = modelStore()->modelForRecord(record);
  char * initialText = nullptr;
  char initialTextContent[TextField::maxBufferSize()];
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    switch (sequenceDefinition) {
      case 0:
        sequence->text(initialTextContent, sizeof(initialTextContent));
        break;
      case 1:
        sequence->firstInitialConditionText(initialTextContent, sizeof(initialTextContent));
        break;
      default:
        sequence->secondInitialConditionText(initialTextContent, sizeof(initialTextContent));
        break;
    }
    initialText = initialTextContent;
  }
  App * myApp = (App *)app();
  InputViewController * inputController = myApp->inputViewController();
  // Invalidate the sequences context cache
  static_cast<App *>(app())->localContext()->resetCache();
  switch (sequenceDefinition) {
    case 0:
      inputController->edit(this, event, this, initialText,
        [](void * context, void * sender){
        ListController * myController = static_cast<ListController *>(context);
        InputViewController * myInputViewController = (InputViewController *)sender;
        const char * textBody = myInputViewController->textBody();
        return myController->editSelectedRecordWithText(textBody);
        },
        [](void * context, void * sender){
        return true;
      });
      break;
  case 1:
    inputController->edit(this, event, this, initialText,
      [](void * context, void * sender){
      ListController * myController = static_cast<ListController *>(context);
      InputViewController * myInputViewController = (InputViewController *)sender;
      const char * textBody = myInputViewController->textBody();
      return myController->editInitialConditionOfSelectedRecordWithText(textBody, true);
      },
      [](void * context, void * sender){
      return true;
    });
    break;
  default:
    inputController->edit(this, event, this, initialText,
      [](void * context, void * sender){
      ListController * myController = static_cast<ListController *>(context);
      InputViewController * myInputViewController = (InputViewController *)sender;
      const char * textBody = myInputViewController->textBody();
      return myController->editInitialConditionOfSelectedRecordWithText(textBody, false);
      },
      [](void * context, void * sender){
      return true;
    });
  }
}

bool ListController::editInitialConditionOfSelectedRecordWithText(const char * text, bool firstInitialCondition) {
  // Reset memoization of the selected cell which always corresponds to the k_memoizedCellsCount/2 memoized cell
  resetMemoizationForIndex(k_memoizedCellsCount/2);
  Ion::Storage::Record record = modelStore()->recordAtIndex(modelIndexForRow(selectedRow()));
  Sequence * sequence = modelStore()->modelForRecord(record);
  Ion::Storage::Record::ErrorStatus error = firstInitialCondition? sequence->setFirstInitialConditionContent(text) : sequence->setSecondInitialConditionContent(text);
  return (error == Ion::Storage::Record::ErrorStatus::None);
}

TextFieldDelegateApp * ListController::textFieldDelegateApp() {
  return (App *)app();
}

ExpressionFieldDelegateApp * ListController::expressionFieldDelegateApp() {
  return (App *)app();
}

InputEventHandlerDelegateApp * ListController::inputEventHandlerDelegateApp() {
  return (App *)app();
}

ListParameterController * ListController::parameterController() {
  return &m_parameterController;
}

int ListController::maxNumberOfDisplayableRows() {
  return k_maxNumberOfRows;
}

FunctionTitleCell * ListController::titleCells(int index) {
  assert(index >= 0 && index < k_maxNumberOfRows);
  return &m_sequenceTitleCells[index];
}

HighlightCell * ListController::expressionCells(int index) {
  assert(index >= 0 && index < k_maxNumberOfRows);
  return &m_expressionCells[index];
}

void ListController::willDisplayTitleCellAtIndex(HighlightCell * cell, int j) {
  assert(j>=0 && j < k_maxNumberOfRows);
  SequenceTitleCell * myCell = (SequenceTitleCell *)cell;
  // Update the corresponding expression cell to get its baseline
  willDisplayExpressionCellAtIndex(m_selectableTableView.cellAtLocation(1, j), j);
  myCell->setBaseline(baseline(j));
  // Set the layout
  Ion::Storage::Record record = modelStore()->recordAtIndex(modelIndexForRow(j));
  Sequence * sequence = modelStore()->modelForRecord(record);
  if (sequenceDefinitionForRow(j) == 0) {
    myCell->setLayout(sequence->definitionName());
  }
  if (sequenceDefinitionForRow(j) == 1) {
    myCell->setLayout(sequence->firstInitialConditionName());
  }
  if (sequenceDefinitionForRow(j) == 2) {
    myCell->setLayout(sequence->secondInitialConditionName());
  }
  // Set the color
  KDColor nameColor = sequence->isActive() ? sequence->color() : Palette::GreyDark;
  myCell->setColor(nameColor);
}

void ListController::willDisplayExpressionCellAtIndex(HighlightCell * cell, int j) {
  FunctionExpressionCell * myCell = (FunctionExpressionCell *)cell;
  Ion::Storage::Record record = modelStore()->recordAtIndex(modelIndexForRow(j));
  Sequence * sequence = modelStore()->modelForRecord(record);
  if (sequenceDefinitionForRow(j) == 0) {
    myCell->setLayout(sequence->layout());
  }
  if (sequenceDefinitionForRow(j) == 1) {
    myCell->setLayout(sequence->firstInitialConditionLayout());
  }
  if (sequenceDefinitionForRow(j) == 2) {
    myCell->setLayout(sequence->secondInitialConditionLayout());
  }
  bool active = sequence->isActive();
  KDColor textColor = active ? KDColorBlack : Palette::GreyDark;
  myCell->setTextColor(textColor);
}

int ListController::modelIndexForRow(int j) {
  if (j < 0) {
    return j;
  }
  if (isAddEmptyRow(j)) {
    return modelIndexForRow(j-1)+1;
  }
  int rowIndex = 0;
  int sequenceIndex = -1;
  do {
    sequenceIndex++;
    Sequence * sequence = modelStore()->modelForRecord(modelStore()->recordAtIndex(sequenceIndex));
    rowIndex += sequence->numberOfElements();
  } while (rowIndex <= j);
  return sequenceIndex;
}

bool ListController::isAddEmptyRow(int j) {
  return modelStore()->numberOfModels() < modelStore()->maxNumberOfModels() && j == numberOfRows() - 1;
}

int ListController::sequenceDefinitionForRow(int j) {
  if (j < 0) {
    return j;
  }
  if (isAddEmptyRow(j)) {
    return 0;
  }
  int rowIndex = 0;
  int sequenceIndex = -1;
  Sequence * sequence;
  do {
    sequenceIndex++;
    sequence = modelStore()->modelForRecord(modelStore()->recordAtIndex(sequenceIndex));
    rowIndex += sequence->numberOfElements();
  } while (rowIndex <= j);
  assert(sequence);
  return sequence->numberOfElements()-rowIndex+j;
}

void ListController::addEmptyModel() {
  app()->displayModalViewController(&m_typeStackController, 0.f, 0.f, Metric::TabHeight+Metric::ModalTopMargin, Metric::CommonRightMargin, Metric::ModalBottomMargin, Metric::CommonLeftMargin);
}

void ListController::editExpression(Ion::Events::Event event) {
  editExpression(sequenceDefinitionForRow(selectedRow()), event);
}

void ListController::reinitSelectedExpression(ExpiringPointer<SingleExpressionModelHandle> model) {
  // Invalidate the sequences context cache
  static_cast<App *>(app())->localContext()->resetCache();
  Sequence * sequence = static_cast<Sequence *>(model.pointer());
  switch (sequenceDefinitionForRow(selectedRow())) {
    case 1:
      if (sequence->firstInitialConditionExpressionClone().isUninitialized()) {
        return;
      }
      sequence->setFirstInitialConditionContent("");
      break;
    case 2:
      if (sequence->secondInitialConditionExpressionClone().isUninitialized()) {
        return;
      }
      sequence->setSecondInitialConditionContent("");
      break;
    default:
      if (sequence->expressionClone().isUninitialized()) {
        return;
      }
      sequence->setContent("");
      break;
  }
  selectableTableView()->reloadData();
}

bool ListController::removeModelRow(Ion::Storage::Record record) {
  Shared::StorageFunctionListController::removeModelRow(record);
  // Invalidate the sequences context cache
  static_cast<App *>(app())->localContext()->resetCache();
  return true;
}

}
